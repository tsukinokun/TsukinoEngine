--------------------------------------------------------------------------------
-- gen_manifest.lua
--
-- premake5 のカスタムアクション "gen-manifest" を定義する。
--
--   vendor\premake5.exe gen-manifest
--
-- エンジンとゲームのソースを静的解析し、次の2つを Docs/ に生成する。
--
--   Docs/agent-manifest.json : 機械可読の契約ファイル。外部ツール（MCPサーバー等）は
--                              エンジンのC++を直接パースせず、必ずこれだけを読む。
--   Docs/components.md       : 上記を人間／AIエージェント向けに整形した一覧。
--
-- 反射機構は使わない。エンジンの唯一の情報源は次の3つの定型パターンであり、
-- どれも1行で書かれるためLuaのパターンマッチだけで抽出できる。
--
--   1. struct 定義         : ファイル名と一致する struct が「コンポーネント」
--   2. cereal::make_nvp("キー", 変数.フィールド)  : JSONに出るフィールドとその順序
--   3. RegisterComponent<型>("登録名")            : 型 と Prefab JSON上の名前の対応
--------------------------------------------------------------------------------

-- マニフェストのスキーマ版。出力の構造を変えたら必ず上げること。
-- 外部ツールはこの値を見て非互換を検出する
local SCHEMA_VERSION = "1.0.0"

--------------------------------------------------------------------------------
-- ユーティリティ
--------------------------------------------------------------------------------

-- ファイルを文字列として読み込みます。
-- @param  [in] filePath 読み込むファイルのパス
-- @return 内容の文字列。開けなければ nil
local function ReadFile(filePath)
	local handle = io.open(filePath, "rb")
	if not handle then
		return nil
	end
	local contents = handle:read("*a")
	handle:close()
	-- UTF-8 BOM を除去（エンジンのヘッダには BOM 付きのものが混在する）
	contents = contents:gsub("^\239\187\191", "")
	return contents
end

-- 文字列をファイルへ書き出します。
local function WriteFile(filePath, contents)
	os.mkdir(path.getdirectory(filePath))
	local handle = io.open(filePath, "wb")
	if not handle then
		error("Failed to open for write: " .. filePath)
	end
	handle:write(contents)
	handle:close()
end

-- C++ のコメントを空白に置き換えます。
-- コメント内の記述を宣言として誤検出しないために使います。
local function StripComments(source)
	source = source:gsub("/%*.-%*/", " ")
	source = source:gsub("//[^\r\n]*", "")
	return source
end

-- 前後の空白を除去します。
local function Trim(text)
	return (text:gsub("^%s+", ""):gsub("%s+$", ""))
end

-- パスを常にスラッシュ区切りに正規化します（JSONの出力を環境非依存にするため）。
local function ToSlash(filePath)
	return (filePath:gsub("\\", "/"))
end

--------------------------------------------------------------------------------
-- 最小限の JSON 出力
--
-- premake に JSON ライブラリは同梱されていないため自前で書く。
-- キーの順序を自分で決められることが重要で、これにより生成結果が決定的になり、
-- 再生成しても git の差分が出ない
--------------------------------------------------------------------------------

local Json = {}

function Json.EscapeString(text)
	text = text:gsub("\\", "\\\\")
	text = text:gsub("\"", "\\\"")
	text = text:gsub("\r", [[\r]])
	text = text:gsub("\n", [[\n]])
	text = text:gsub("\t", [[\t]])
	return text
end

-- 値を JSON へ整形します。
-- @param [in] value  出力する値。table の場合 value.__order があればその順で出力する
-- @param [in] indent インデントの深さ
function Json.Encode(value, indent)
	indent = indent or 0
	local pad = string.rep("    ", indent)
	local padInner = string.rep("    ", indent + 1)

	if type(value) == "string" then
		return "\"" .. Json.EscapeString(value) .. "\""
	elseif type(value) == "number" then
		return tostring(value)
	elseif type(value) == "boolean" then
		return value and "true" or "false"
	elseif type(value) == "nil" then
		return "null"
	elseif type(value) == "table" then
		-- 配列
		if #value > 0 or value.__array then
			if #value == 0 then
				return "[]"
			end
			local parts = {}
			for _, item in ipairs(value) do
				parts[#parts + 1] = padInner .. Json.Encode(item, indent + 1)
			end
			return "[\n" .. table.concat(parts, ",\n") .. "\n" .. pad .. "]"
		end
		-- オブジェクト
		local keys = value.__order
		if not keys then
			keys = {}
			for key in pairs(value) do
				if key ~= "__order" and key ~= "__array" then
					keys[#keys + 1] = key
				end
			end
			table.sort(keys)
		end
		if #keys == 0 then
			return "{}"
		end
		local parts = {}
		for _, key in ipairs(keys) do
			parts[#parts + 1] = padInner .. "\"" .. Json.EscapeString(key) .. "\": " .. Json.Encode(value[key], indent + 1)
		end
		return "{\n" .. table.concat(parts, ",\n") .. "\n" .. pad .. "}"
	end
	error("Unsupported value type for JSON: " .. type(value))
end

--------------------------------------------------------------------------------
-- C++ 型 -> Prefab JSON 表現の対応
--------------------------------------------------------------------------------

local JSON_SHAPE = {
	["hlslpp::float2"]           = "{ \"x\", \"y\" }",
	["hlslpp::float3"]           = "{ \"x\", \"y\", \"z\" }",
	["hlslpp::float4"]           = "{ \"x\", \"y\", \"z\", \"w\" }",
	["hlslpp::quaternion"]       = "{ \"x\", \"y\", \"z\", \"w\" }",
	["Tsukino::ECS::EntityRef"]  = "\"#EntityName\"",
	["Tsukino::Asset::AssetRef"] = "\"path/to/asset\"",
	["std::string"]              = "string",
	["std::wstring"]             = "string",
	["bool"]                     = "true / false",
	["int"]                      = "number",
	["float"]                    = "number",
	["double"]                   = "number",
	["u8"]                       = "number",
	["u16"]                      = "number",
	["u32"]                      = "number",
	["u64"]                      = "number",
	["s8"]                       = "number",
	["s16"]                      = "number",
	["s32"]                      = "number",
	["s64"]                      = "number",
	["f32"]                      = "number",
}

-- C++ の型名から Prefab JSON 上の表現を求めます。
-- @param [in] cppType   C++ の型名
-- @param [in] enumNames そのヘッダで定義されている enum class 名の集合
local function ResolveJsonShape(cppType, enumNames)
	local shape = JSON_SHAPE[cppType]
	if shape then
		return shape
	end
	-- 名前空間を落として再検索（同一名前空間内では修飾なしで書かれるため）
	local unqualified = cppType:match("([%w_]+)$")
	if unqualified then
		if enumNames[unqualified] or enumNames[cppType] then
			return "number（enum の整数値）"
		end
		shape = JSON_SHAPE[unqualified]
		if shape then
			return shape
		end
	end
	return "?"
end

--------------------------------------------------------------------------------
-- 解析
--------------------------------------------------------------------------------

-- ヘッダから struct 定義（コンポーネント）を抽出します。
--
-- 「ファイル名と同名の struct だけをコンポーネントとみなす」という判定は、
-- エンジンの命名規約（クラスを定義するヘッダはクラス名と同じファイル名にする）に
-- そのまま乗っている。これにより SpringBoneComponent.hpp 内の ColliderDef /
-- ChainDef のような補助構造体が自動的に除外される。
--
-- @param  [in] headerPath ヘッダのパス
-- @return コンポーネント情報の table。該当しなければ nil
local function ParseComponentHeader(headerPath)
	local raw = ReadFile(headerPath)
	if not raw then
		return nil
	end

	local structName = path.getbasename(headerPath)
	-- コンポーネントかタグのみを対象にする
	if not (structName:match("Component$") or structName:match("Tag$")) then
		return nil
	end

	local source = StripComments(raw)

	-- struct 本体（前方宣言は "{" を持たないため自動的に読み飛ばされる）
	local body = source:match("struct%s+" .. structName .. "%s*(%b{})")
	if not body then
		return nil
	end

	-- 同一ヘッダ内の enum class を集める（フィールド型の判定に使う）
	local enumNames = {}
	for enumName in source:gmatch("enum%s+class%s+([%w_]+)") do
		enumNames[enumName] = true
	end


	-- 囲っている名前空間（例: Tsukino::BuiltIn::ECS）。完全修飾型名の組み立てに使う
	local namespaceName = source:match("namespace%s+([%w_:]+)%s*{")
	-- フィールドを宣言順に取り出す
	local fields = {}
	local fieldByName = {}
	for line in body:gmatch("[^\r\n]+") do
		local declaration = line:match("^%s*(.-)%s*;")
		if declaration and declaration ~= "" then
			-- 既定値を分離する
			local lhs, defaultValue = declaration:match("^(.-)%s*=%s*(.+)$")
			lhs = lhs or declaration
			-- 型と名前に分ける。関数宣言は lhs が ")" で終わるためここで落ちる
			local cppType, fieldName = lhs:match("^%s*([%w_:<>,%s%*&]-)%s+([%w_]+)%s*$")
			if cppType and fieldName then
				cppType = Trim(cppType)
				-- アクセス指定子や static / using などは対象外
				if cppType ~= "" and not cppType:match("^using") and not cppType:match("^typedef") then
					local field = {
						name    = fieldName,
						cppType = cppType,
						default = defaultValue and Trim(defaultValue) or nil,
					}
					fields[#fields + 1] = field
					fieldByName[fieldName] = field
				end
			end
		end
	end

	return {
		name          = structName,
		qualifiedName = namespaceName and (namespaceName .. "::" .. structName) or structName,
		headerPath    = headerPath,
		fields        = fields,
		fieldByName   = fieldByName,
		enumNames     = enumNames,
	}
end

-- *Serialization.hpp から「JSON に出るキーとその順序」を抽出します。
-- @param  [in] serializationPath シリアライズヘッダのパス
-- @return 見つかった save 定義の配列。空なら空配列
local function ParseSerializationHeaders(serializationPath)
	local results = {}

	local raw = ReadFile(serializationPath)
	if not raw then
		return results
	end
	local source = StripComments(raw)

	-- 1ファイルに複数の save() が書かれることがある
	-- （コンポーネント本体と、そのメンバに使う補助型の両方など）。
	-- すべて拾い、コンポーネント名と一致したものだけを後段で採用する
	for signature, body in source:gmatch("void%s+save%s*(%b())%s*(%b{})") do
		local typeName = signature:match("const%s+([%w_:]+)%s*&")
		if typeName then
			local keys = {}
			local keyToField = {}
			for jsonKey, expression in body:gmatch("make_nvp%s*%(%s*\"([^\"]+)\"%s*,%s*([^,%)]+)%s*%)") do
				keys[#keys + 1] = jsonKey
				-- "sprite.textureHandle" -> "textureHandle"
				keyToField[jsonKey] = Trim(expression):match("([%w_]+)%s*$")
			end

			results[#results + 1] = {
				typeName   = typeName,
				shortName  = typeName:match("([%w_]+)$"),
				keys       = keys,
				keyToField = keyToField,
				filePath   = serializationPath,
			}
		end
	end

	return results
end

-- ソースから RegisterComponent<型>("登録名") を抽出します。
-- @param [in]     sourcePath 走査する .cpp のパス
-- @param [in,out] outResult  短い型名 -> { registeredName, site } を書き込む table
local function CollectRegistrations(sourcePath, outResult)
	local raw = ReadFile(sourcePath)
	if not raw then
		return
	end
	local source = StripComments(raw)
	for typeName, registeredName in source:gmatch("RegisterComponent%s*<%s*([%w_:]+)%s*>%s*%(%s*\"([^\"]+)\"") do
		local shortName = typeName:match("([%w_]+)$")
		outResult[shortName] = {
			typeName       = typeName,
			registeredName = registeredName,
			site           = sourcePath,
		}
	end
end

--------------------------------------------------------------------------------
-- 走査対象の収集
--------------------------------------------------------------------------------

-- 走査から除外するトップレベルのディレクトリ名
local SKIP_DIRS = {
	["External"] = true, [".build"] = true, [".git"] = true, [".vs"] = true,
	["bin"] = true, ["bin-int"] = true, [".claude"] = true, ["vendor"] = true,
	["Tools"] = true, ["Docs"] = true, ["html"] = true, ["xml"] = true,
	["image"] = true, [".kilo"] = true,
}

-- 指定ディレクトリ配下から、拡張子の一致するファイルを再帰的に集めます。
local function GatherFiles(rootDir, pattern, outFiles)
	for _, filePath in ipairs(os.matchfiles(path.join(rootDir, pattern))) do
		outFiles[#outFiles + 1] = filePath
	end
end

-- ルート直下の各ディレクトリを走査対象に加えます（除外リストを尊重する）。
local function GatherFromRoot(rootDir, outHeaders, outSources)
	for _, dirPath in ipairs(os.matchdirs(path.join(rootDir, "*"))) do
		local dirName = path.getname(dirPath)
		-- Tsukino.Sandbox はエンジン単体ビルド用のサンプルなので対象外
		if not SKIP_DIRS[dirName] and dirName ~= "Tsukino.Sandbox" then
			GatherFiles(dirPath, "**.hpp", outHeaders)
			GatherFiles(dirPath, "**.h", outHeaders)
			GatherFiles(dirPath, "**.cpp", outSources)
		end
	end
end

--------------------------------------------------------------------------------
-- Markdown の生成
--------------------------------------------------------------------------------

-- 1コンポーネント分の Markdown を組み立てます。
local function BuildComponentSection(entry, lines)
	lines[#lines + 1] = "### " .. entry.name
	lines[#lines + 1] = ""
	lines[#lines + 1] = "- 型: `" .. entry.typeName .. "`"
	if entry.registered then
		lines[#lines + 1] = "- Prefab 登録名: `" .. entry.registeredName .. "`"
	else
		lines[#lines + 1] = "- Prefab 登録名: **未登録**（`RegisterComponent<" .. entry.name .. ">(...)` の呼び出しが無い）"
	end
	lines[#lines + 1] = "- 定義: `" .. entry.header .. "`"
	if entry.serializationHeader then
		lines[#lines + 1] = "- シリアライズ: `" .. entry.serializationHeader .. "`"
	end
	lines[#lines + 1] = ""

	if entry.fields and #entry.fields > 0 then
		lines[#lines + 1] = "| JSON キー | C++ 型 | JSON 表現 | 既定値 |"
		lines[#lines + 1] = "|---|---|---|---|"
		for _, field in ipairs(entry.fields) do
			lines[#lines + 1] = string.format("| `%s` | `%s` | %s | %s |",
				field.jsonKey,
				field.cppType,
				field.jsonShape,
				field.default and ("`" .. field.default .. "`") or "—")
		end
	else
		lines[#lines + 1] = "JSON に出るフィールドは無い（タグ、または save() が空）。"
	end
	lines[#lines + 1] = ""
end

-- components.md の全文を組み立てます。
local function BuildMarkdown(components, stats)
	local lines = {}
	lines[#lines + 1] = "# コンポーネント一覧"
	lines[#lines + 1] = ""
	lines[#lines + 1] = "**このファイルは自動生成です。直接編集しないでください。**"
	lines[#lines + 1] = ""
	lines[#lines + 1] = "再生成: `" .. [[vendor\premake5.exe]] .. " gen-manifest`（または `generate-docs.bat`）"
	lines[#lines + 1] = ""
	lines[#lines + 1] = string.format(
		"コンポーネント %d 個（Prefab 登録済み %d 個 / シリアライズ定義あり %d 個）",
		stats.total, stats.registered, stats.serializable)
	lines[#lines + 1] = ""
	lines[#lines + 1] = "## 読み方"
	lines[#lines + 1] = ""
	lines[#lines + 1] = "- **JSON キー** … `cereal::make_nvp` の第1引数。Prefab JSON にはこの名前で書く"
	lines[#lines + 1] = "- **JSON 表現** … Prefab JSON 上での値の形。`?` は自動判定できなかったもので、"
	lines[#lines + 1] = "  その場合は定義ヘッダを直接確認する"
	lines[#lines + 1] = "- コンポーネント JSON のルートキーは **Prefab 登録名**そのもの"
	lines[#lines + 1] = "- `EntityRef` は `\"#EntityName\"`。名前はグループファイル（`*.Group.json`）の `key` を指す"
	lines[#lines + 1] = "- `AssetRef` は作業ディレクトリ相対のパス文字列"
	lines[#lines + 1] = "- **シリアライズ定義が無いコンポーネントは Prefab に書けない**。コードから `AddComponent` する"
	lines[#lines + 1] = "- **ここに載るのはエンジン組み込みのコンポーネントだけ**。"
	lines[#lines + 1] = "  ゲーム固有のコンポーネントはゲーム側リポジトリで定義・登録するため含まれない"
	lines[#lines + 1] = "- 登録済みだがシリアライズ定義が無いものは、名前だけ登録されていて Prefab からは生成できない"
	lines[#lines + 1] = ""

	local registeredList = {}
	local otherList = {}
	for _, entry in ipairs(components) do
		if entry.registered and entry.serializable then
			registeredList[#registeredList + 1] = entry
		else
			otherList[#otherList + 1] = entry
		end
	end

	lines[#lines + 1] = "## Prefab で使えるコンポーネント"
	lines[#lines + 1] = ""
	lines[#lines + 1] = "（登録済み かつ シリアライズ定義あり）"
	lines[#lines + 1] = ""
	for _, entry in ipairs(registeredList) do
		BuildComponentSection(entry, lines)
	end

	lines[#lines + 1] = "## Prefab から生成できないコンポーネント"
	lines[#lines + 1] = ""
	lines[#lines + 1] = "登録もしくはシリアライズ定義が無いため、コードから直接扱う。"
	lines[#lines + 1] = ""
	lines[#lines + 1] = "| コンポーネント | 登録 | シリアライズ | 定義 |"
	lines[#lines + 1] = "|---|---|---|---|"
	for _, entry in ipairs(otherList) do
		lines[#lines + 1] = string.format("| `%s` | %s | %s | `%s` |",
			entry.name,
			entry.registered and ("`" .. entry.registeredName .. "`") or "—",
			entry.serializable and "あり" or "—",
			entry.header)
	end
	lines[#lines + 1] = ""

	return table.concat(lines, "\r\n")
end

--------------------------------------------------------------------------------
-- アクション本体
--------------------------------------------------------------------------------

newaction {
	trigger     = "gen-manifest",
	description = "エンジンとゲームを静的解析し Docs/agent-manifest.json と Docs/components.md を生成する",

	execute = function()
		local engineRoot  = _TSUKINO_ENGINE_ROOT
		if not engineRoot then
			error("gen-manifest: _TSUKINO_ENGINE_ROOT が未設定です。premake5.lua から dofile してください。")
		end
		engineRoot = path.getabsolute(engineRoot)

		-- 走査範囲はエンジンのみに限定する。
		-- 出力はエンジンリポジトリの Docs/ にコミットされるため、
		-- 特定ゲームのコンポーネントが混ざると、そのエンジンを使う他のプロジェクトと
		-- 内容が食い違ってしまう。ゲーム固有のコンポーネントはゲーム側の責務とする
		local headers = {}
		local sources = {}
		GatherFromRoot(engineRoot, headers, sources)

		----------------------------------------------------------------------
		-- 解析する
		----------------------------------------------------------------------
		local componentsByName    = {}
		local serializationByName = {}
		local registrationByName  = {}

		for _, headerPath in ipairs(headers) do
			if path.getbasename(headerPath):match("Serialization$") then
				-- 1ファイルに複数の save() があり得るのですべて登録する
				for _, serialization in ipairs(ParseSerializationHeaders(headerPath)) do
					serializationByName[serialization.shortName] = serialization
				end
			else
				local component = ParseComponentHeader(headerPath)
				if component then
					componentsByName[component.name] = component
				end
			end
		end

		for _, sourcePath in ipairs(sources) do
			CollectRegistrations(sourcePath, registrationByName)
		end

		----------------------------------------------------------------------
		-- 突き合わせる
		----------------------------------------------------------------------
		local names = {}
		for name in pairs(componentsByName) do
			names[#names + 1] = name
		end
		-- 出力を決定的にするため名前順に固定する
		table.sort(names)

		local entries      = { __array = true }
		local countTotal   = 0
		local countReg     = 0
		local countSerial  = 0

		for _, name in ipairs(names) do
			local component     = componentsByName[name]
			local serialization = serializationByName[name]
			local registration  = registrationByName[name]

			countTotal = countTotal + 1
			if registration then countReg = countReg + 1 end
			if serialization then countSerial = countSerial + 1 end

			-- JSON に出るフィールドを、make_nvp に書かれた順で組み立てる。
			-- 型と既定値はコンポーネントヘッダ側の宣言から引く
			local fields = { __array = true }
			if serialization then
				for _, jsonKey in ipairs(serialization.keys) do
					local fieldName = serialization.keyToField[jsonKey] or jsonKey
					local declared  = component.fieldByName[fieldName]
					local cppType   = declared and declared.cppType or "?"
					fields[#fields + 1] = {
						__order   = { "jsonKey", "field", "cppType", "jsonShape", "default" },
						jsonKey   = jsonKey,
						field     = fieldName,
						cppType   = cppType,
						jsonShape = ResolveJsonShape(cppType, component.enumNames),
						default   = declared and declared.default or nil,
					}
				end
			end

			local headerRel = ToSlash(path.getrelative(engineRoot, component.headerPath))

			entries[#entries + 1] = {
				__order             = {
					"name", "typeName", "registered", "registeredName",
					"serializable", "header", "serializationHeader", "registrationSite", "fields",
				},
				name                = name,
				typeName            = (registration and registration.typeName)
				                      or (serialization and serialization.typeName)
				                      or component.qualifiedName,
				registered          = registration ~= nil,
				registeredName      = registration and registration.registeredName or nil,
				serializable        = serialization ~= nil,
				header              = headerRel,
				serializationHeader = serialization and ToSlash(path.getrelative(engineRoot, serialization.filePath)) or nil,
				registrationSite    = registration and ToSlash(path.getrelative(engineRoot, registration.site)) or nil,
				fields              = fields,
			}
		end

		----------------------------------------------------------------------
		-- 未知の登録名（コンポーネント定義が見つからなかったもの）を検出する
		----------------------------------------------------------------------
		local orphanRegistrations = { __array = true }
		for name, registration in pairs(registrationByName) do
			if not componentsByName[name] then
				orphanRegistrations[#orphanRegistrations + 1] = {
					__order        = { "typeName", "registeredName", "registrationSite" },
					typeName       = registration.typeName,
					registeredName = registration.registeredName,
					registrationSite = ToSlash(path.getrelative(engineRoot, registration.site)),
				}
			end
		end
		table.sort(orphanRegistrations, function(a, b) return a.typeName < b.typeName end)

		----------------------------------------------------------------------
		-- 書き出す
		----------------------------------------------------------------------
		local manifest = {
			__order = { "schemaVersion", "generator", "engineRoot", "prefab", "stats", "components", "unresolvedRegistrations" },
			schemaVersion = SCHEMA_VERSION,
			generator     = "premake5 gen-manifest",
			engineRoot    = ".",
			prefab        = {
				__order              = { "groupFileSuffix", "prefabIndexFile", "componentRootKey", "entityRefPrefix", "pathBase" },
				groupFileSuffix      = ".Group.json",
				prefabIndexFile      = "Prefab.json",
				componentRootKey     = "registeredName",
				entityRefPrefix      = "#",
				pathBase             = "working directory (Debug: repository root / Release: next to the exe)",
			},
			stats = {
				__order      = { "components", "registered", "serializable" },
				components   = countTotal,
				registered   = countReg,
				serializable = countSerial,
			},
			components              = entries,
			unresolvedRegistrations = orphanRegistrations,
		}

		local docsDir      = path.join(engineRoot, "Docs")
		local manifestPath = path.join(docsDir, "agent-manifest.json")
		local markdownPath = path.join(docsDir, "components.md")

		WriteFile(manifestPath, Json.Encode(manifest, 0) .. "\n")
		WriteFile(markdownPath, BuildMarkdown(entries, {
			total        = countTotal,
			registered   = countReg,
			serializable = countSerial,
		}) .. "\r\n")

		print(string.format("gen-manifest: コンポーネント %d 個（登録済み %d / シリアライズ可 %d）",
			countTotal, countReg, countSerial))
		if #orphanRegistrations > 0 then
			print(string.format("gen-manifest: 定義を特定できなかった登録が %d 件あります", #orphanRegistrations))
		end
		print("gen-manifest: " .. ToSlash(path.getrelative(engineRoot, manifestPath)))
		print("gen-manifest: " .. ToSlash(path.getrelative(engineRoot, markdownPath)))
	end,
}
