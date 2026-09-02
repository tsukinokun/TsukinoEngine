--------------------------------------------------------------------------------
-- gen_api_digest.lua
--
-- premake5 のカスタムアクション "gen-api-digest" を定義する。
--
--   doxygen Doxyfile          （先に XML を生成しておく）
--   vendor\premake5.exe gen-api-digest
--
-- Doxygen が出力した XML から、公開 API の索引 Docs/api-digest.md を生成する。
-- ヘッダを直接読ませる代わりに、シグネチャと1行要約だけを引ける状態にするのが目的。
--
-- 出力は2部構成にしている。
--   1. 主要な型  … ゲームプログラマが実際に触る型。メンバを全部並べる
--   2. 全索引    … モジュール別の一覧。メンバ名だけを並べ、grep の入口にする
--------------------------------------------------------------------------------

-- 「主要な型」として全メンバを展開する型（名前空間を除いた末尾の名前で判定する）
local PRIMARY_TYPES = {
	"EngineIntegration", "EngineContext", "EngineAPI",
	"GameSceneBase", "GameSceneManager", "Scene",
	"Registry", "ISystem", "EntityRef", "EventBus",
	"PrefabFactory", "AssetManager", "AssetRef", "AssetHandle",
	"InputSystem", "Window", "Log", "Path",
	"Renderer", "AudioManager",
}

--------------------------------------------------------------------------------
-- XML の下ごしらえ
--------------------------------------------------------------------------------

-- XML の実体参照を元の文字へ戻します。
local function DecodeXmlEntities(text)
	text = text:gsub("&lt;", "<")
	text = text:gsub("&gt;", ">")
	text = text:gsub("&quot;", '"')
	text = text:gsub("&apos;", "'")
	text = text:gsub("&amp;", "&")
	return text
end

-- タグを取り除き、1行の平文にします。
local function FlattenXmlText(fragment)
	if not fragment then
		return ""
	end
	fragment = fragment:gsub("<[^>]*>", " ")
	fragment = DecodeXmlEntities(fragment)
	fragment = fragment:gsub("%s+", " ")
	fragment = fragment:gsub("^%s+", ""):gsub("%s+$", "")
	return fragment
end

-- Markdown の表を壊す文字を退避します。
local function EscapeForTable(text)
	return (text:gsub("|", [[\|]]))
end

--------------------------------------------------------------------------------
-- 解析
--------------------------------------------------------------------------------

-- クラス／構造体の XML を1つ解析します。
-- @param  [in] xmlPath XML ファイルのパス
-- @return { name, kind, module, header, functions, variables } / 対象外なら nil
local function ParseCompoundXml(xmlPath)
	local handle = io.open(xmlPath, "rb")
	if not handle then
		return nil
	end
	local source = handle:read("*a")
	handle:close()

	local compoundName = source:match("<compoundname>([^<]+)</compoundname>")
	if not compoundName then
		return nil
	end
	compoundName = DecodeXmlEntities(compoundName)

	-- エンジンの公開 API だけを対象にする（Sandbox のサンプルなどは除外）
	if not compoundName:match("^Tsukino::") then
		return nil
	end

	local kind = source:match('<compounddef[^>]-kind="([%w]+)"') or "class"

	-- 定義元ヘッダ。モジュール名はそのパスの先頭から取る
	local headerFile = source:match('<location file="([^"]+)"')
	local moduleName = headerFile and headerFile:match("^([^/]+)") or "?"

	local functions = {}
	local variables = {}

	for memberBlock in source:gmatch("<memberdef.-</memberdef>") do
		local prot = memberBlock:match('prot="([%w]+)"')
		if prot == "public" then
			-- テンプレート引数リストは <type> より前に現れ、その中にも <type> があるため、
			-- 先に切り離しておかないと戻り値として "typename T" を拾ってしまう
			local templateBlock = memberBlock:match("<templateparamlist>(.-)</templateparamlist>")
			local plainBlock    = memberBlock:gsub("<templateparamlist>.-</templateparamlist>", "")

			local templatePrefix = ""
			if templateBlock then
				local templateParams = {}
				for paramBlock in templateBlock:gmatch("<param>(.-)</param>") do
					local paramType = FlattenXmlText(paramBlock:match("<type>(.-)</type>"))
					local paramName = paramBlock:match("<declname>([^<]*)</declname>")
					if paramName then
						templateParams[#templateParams + 1] = paramType .. " " .. DecodeXmlEntities(paramName)
					elseif paramType ~= "" then
						templateParams[#templateParams + 1] = paramType
					end
				end
				if #templateParams > 0 then
					templatePrefix = "template <" .. table.concat(templateParams, ", ") .. "> "
				end
			end

			local memberKind = plainBlock:match('<memberdef%s+kind="([%w-]+)"')
			local name       = plainBlock:match("<name>([^<]*)</name>")
			local typeText   = FlattenXmlText(plainBlock:match("<type>(.-)</type>"))
			local args       = DecodeXmlEntities(plainBlock:match("<argsstring>([^<]*)</argsstring>") or "")
			local brief      = FlattenXmlText(plainBlock:match("<briefdescription>(.-)</briefdescription>"))

			if name then
				name = DecodeXmlEntities(name)
				if memberKind == "function" then
					functions[#functions + 1] = {
						name      = name,
						signature = templatePrefix .. ((typeText ~= "") and (typeText .. " ") or "") .. name .. args,
						brief     = brief,
					}
				elseif memberKind == "variable" then
					variables[#variables + 1] = {
						name      = name,
						signature = ((typeText ~= "") and (typeText .. " ") or "") .. name,
						brief     = brief,
					}
				end
			end
		end
	end

	return {
		name      = compoundName,
		shortName = compoundName:match("([%w_]+)$"),
		kind      = kind,
		module    = moduleName,
		header    = headerFile or "?",
		functions = functions,
		variables = variables,
	}
end

--------------------------------------------------------------------------------
-- Markdown の生成
--------------------------------------------------------------------------------

-- 主要な型の節（メンバを全部並べる）を組み立てます。
local function BuildPrimarySection(entry, lines)
	lines[#lines + 1] = "### " .. entry.name
	lines[#lines + 1] = ""
	lines[#lines + 1] = "`" .. entry.header .. "`"
	lines[#lines + 1] = ""

	if #entry.variables > 0 then
		lines[#lines + 1] = "**公開メンバ**"
		lines[#lines + 1] = ""
		lines[#lines + 1] = "| メンバ | 説明 |"
		lines[#lines + 1] = "|---|---|"
		for _, member in ipairs(entry.variables) do
			lines[#lines + 1] = "| `" .. EscapeForTable(member.signature) .. "` | " .. EscapeForTable(member.brief) .. " |"
		end
		lines[#lines + 1] = ""
	end

	if #entry.functions > 0 then
		lines[#lines + 1] = "**公開関数**"
		lines[#lines + 1] = ""
		lines[#lines + 1] = "| シグネチャ | 説明 |"
		lines[#lines + 1] = "|---|---|"
		for _, member in ipairs(entry.functions) do
			lines[#lines + 1] = "| `" .. EscapeForTable(member.signature) .. "` | " .. EscapeForTable(member.brief) .. " |"
		end
		lines[#lines + 1] = ""
	end

	if #entry.variables == 0 and #entry.functions == 0 then
		lines[#lines + 1] = "公開メンバなし。"
		lines[#lines + 1] = ""
	end
end

-- api-digest.md の全文を組み立てます。
local function BuildDigest(entries, primaryNames, missingPrimaries)
	local lines = {}
	lines[#lines + 1] = "# API ダイジェスト"
	lines[#lines + 1] = ""
	lines[#lines + 1] = "**このファイルは自動生成です。直接編集しないでください。**"
	lines[#lines + 1] = ""
	lines[#lines + 1] = "再生成: `generate-docs.bat`（doxygen で XML を出してから gen-api-digest）"
	lines[#lines + 1] = ""
	lines[#lines + 1] = "エンジンのヘッダを直接読む前に、まずここを引くこと。"
	lines[#lines + 1] = "コンポーネントと Prefab JSON のフィールドは `components.md` を見る。"
	lines[#lines + 1] = ""

	----------------------------------------------------------------------
	-- 1部: 主要な型
	----------------------------------------------------------------------
	lines[#lines + 1] = "## 主要な型"
	lines[#lines + 1] = ""
	lines[#lines + 1] = "ゲーム側から実際に触る型。メンバを全て展開している。"
	lines[#lines + 1] = ""

	for _, entry in ipairs(entries) do
		if primaryNames[entry.shortName] then
			BuildPrimarySection(entry, lines)
		end
	end

	----------------------------------------------------------------------
	-- 2部: 全索引
	----------------------------------------------------------------------
	lines[#lines + 1] = "## 全公開型の索引"
	lines[#lines + 1] = ""
	lines[#lines + 1] = "メンバ名のみ。詳細が要るときはヘッダを開く。"
	lines[#lines + 1] = ""

	local currentModule = nil
	for _, entry in ipairs(entries) do
		if entry.module ~= currentModule then
			currentModule = entry.module
			lines[#lines + 1] = ""
			lines[#lines + 1] = "### " .. currentModule
			lines[#lines + 1] = ""
		end

		local memberNames = {}
		for _, member in ipairs(entry.variables) do
			memberNames[#memberNames + 1] = member.name
		end
		for _, member in ipairs(entry.functions) do
			memberNames[#memberNames + 1] = member.name .. "()"
		end

		local summary = (#memberNames > 0) and table.concat(memberNames, ", ") or "（公開メンバなし）"
		lines[#lines + 1] = "- **" .. entry.name .. "** — `" .. entry.header .. "`"
		lines[#lines + 1] = "  - " .. summary
	end

	if #missingPrimaries > 0 then
		lines[#lines + 1] = ""
		lines[#lines + 1] = "## 注意"
		lines[#lines + 1] = ""
		lines[#lines + 1] = "主要な型として指定されたが XML に見つからなかったもの:"
		lines[#lines + 1] = ""
		for _, name in ipairs(missingPrimaries) do
			lines[#lines + 1] = "- `" .. name .. "`"
		end
	end

	lines[#lines + 1] = ""
	return table.concat(lines, "\r\n")
end

--------------------------------------------------------------------------------
-- アクション本体
--------------------------------------------------------------------------------

newaction {
	trigger     = "gen-api-digest",
	description = "Doxygen の XML から Docs/api-digest.md を生成する（先に doxygen を実行しておくこと）",

	execute = function()
		local engineRoot = _TSUKINO_ENGINE_ROOT
		if not engineRoot then
			error("gen-api-digest: _TSUKINO_ENGINE_ROOT が未設定です。premake5.lua から dofile してください。")
		end
		engineRoot = path.getabsolute(engineRoot)

		local xmlDir = path.join(engineRoot, "xml")
		if not os.isdir(xmlDir) then
			error("gen-api-digest: " .. xmlDir .. " がありません。先に `doxygen Doxyfile` を実行してください。")
		end

		----------------------------------------------------------------------
		-- XML を読む
		----------------------------------------------------------------------
		local entries = {}
		for _, xmlPath in ipairs(os.matchfiles(path.join(xmlDir, "*.xml"))) do
			local fileName = path.getname(xmlPath)
			if fileName:match("^class") or fileName:match("^struct") then
				local entry = ParseCompoundXml(xmlPath)
				if entry then
					entries[#entries + 1] = entry
				end
			end
		end

		-- モジュール順 → 型名順に固定し、生成結果を決定的にする
		table.sort(entries, function(a, b)
			if a.module ~= b.module then
				return a.module < b.module
			end
			return a.name < b.name
		end)

		----------------------------------------------------------------------
		-- 主要な型の指定が実在するか検証する
		----------------------------------------------------------------------
		local primaryNames = {}
		for _, name in ipairs(PRIMARY_TYPES) do
			primaryNames[name] = true
		end

		local foundPrimaries = {}
		for _, entry in ipairs(entries) do
			if primaryNames[entry.shortName] then
				foundPrimaries[entry.shortName] = true
			end
		end

		local missingPrimaries = {}
		for _, name in ipairs(PRIMARY_TYPES) do
			if not foundPrimaries[name] then
				missingPrimaries[#missingPrimaries + 1] = name
			end
		end

		----------------------------------------------------------------------
		-- 書き出す
		----------------------------------------------------------------------
		local outputPath = path.join(engineRoot, "Docs/api-digest.md")
		local contents   = BuildDigest(entries, primaryNames, missingPrimaries)

		os.mkdir(path.getdirectory(outputPath))
		local handle = io.open(outputPath, "wb")
		if not handle then
			error("gen-api-digest: 書き込みに失敗しました: " .. outputPath)
		end
		handle:write(contents)
		handle:close()

		print(string.format("gen-api-digest: 公開型 %d 件", #entries))
		if #missingPrimaries > 0 then
			print("gen-api-digest: 主要な型のうち未検出: " .. table.concat(missingPrimaries, ", "))
		end
		print("gen-api-digest: Docs/api-digest.md")
	end,
}
