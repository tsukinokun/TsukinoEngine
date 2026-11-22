--------------------------------------------------------------------------------
-- gen_api_digest.lua
--
-- premake5 のカスタムアクション "gen-api-digest" を定義する。
--
--   doxygen Doxyfile          （先に XML を生成しておく）
--   vendor\premake5.exe gen-api-digest
--
-- Doxygen が出力した XML から、公開 API の索引を生成する。
-- ヘッダを直接読ませる代わりに、シグネチャと1行要約だけを引ける状態にするのが目的。
--
-- 出力は2階層に分けている。単一ファイルにまとめると全文で数万トークンになり、
-- 「ヘッダを読ませない」という目的そのものを裏切るため。
--   Docs/api-index.md      … 型・列挙型・型の別名・関数の名前 → ヘッダパスの1行索引。まずここを引く
--   Docs/api/<Module>.md   … モジュール単位の詳細。必要な1本だけ開く
--                             （主要な型はメンバ全展開、それ以外はメンバ名のみ。
--                               列挙型は値と説明、名前空間の関数・定数はシグネチャまで載せる）
--
-- 名前空間の直下にあるもの（列挙型・型の別名・関数・定数）は namespace*.xml から、
-- 公開ヘッダ（Tsukino.<Module>/include/）で宣言されたものだけを拾う。
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

-- memberdef の宣言があるファイルを返します。
-- 宣言がヘッダ・定義が .cpp の関数は、file に .cpp、declfile にヘッダが入るので declfile を優先する。
-- （file だけを見ると、.cpp に中身がある公開関数を「公開ヘッダに無い」と取りこぼす）
local function MemberDeclarationFile(block)
	local location = block:match("<location%s[^>]*>")
	if not location then
		return nil
	end
	return location:match('declfile="([^"]+)"') or location:match('%sfile="([^"]+)"')
end

-- 列挙型の memberdef を1つ解析します（値とその説明まで読む）。
-- @param  [in] block         <memberdef kind="enum"> … </memberdef>
-- @param  [in] ownerName     名前空間またはクラスの完全名（qualifiedname が無いときに使う）
-- @return { name, qualifiedName, brief, header, values = { { name, initializer, brief } } }
local function ParseEnumBlock(block, ownerName)
	local values = {}
	for valueBlock in block:gmatch("<enumvalue.-</enumvalue>") do
		values[#values + 1] = {
			name        = DecodeXmlEntities(valueBlock:match("<name>([^<]*)</name>") or "?"),
			initializer = FlattenXmlText(valueBlock:match("<initializer>(.-)</initializer>")),
			brief       = FlattenXmlText(valueBlock:match("<briefdescription>(.-)</briefdescription>")),
		}
	end

	-- 値の説明と取り違えないよう、値の部分を外してから列挙型自身の名前と説明を読む
	local outer = block:gsub("<enumvalue.-</enumvalue>", "")
	local name  = DecodeXmlEntities(outer:match("<name>([^<]*)</name>") or "?")
	local qualifiedName = outer:match("<qualifiedname>([^<]*)</qualifiedname>")
	return {
		name          = name,
		qualifiedName = qualifiedName and DecodeXmlEntities(qualifiedName) or (ownerName .. "::" .. name),
		brief         = FlattenXmlText(outer:match("<briefdescription>(.-)</briefdescription>")),
		header        = MemberDeclarationFile(outer) or "?",
		values        = values,
	}
end

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

	-- エンジンの公開 API だけを対象にする
	if not compoundName:match("^Tsukino::") then
		return nil
	end

	-- Sandbox はサンプルであってエンジンの公開 API ではない。
	-- Tsukino::Sandbox:: も上の ^Tsukino:: を通ってしまうため、明示的に落とす。
	-- （これが無いと SceneSample1.cpp の中で定義しているテスト専用の
	--   TestLinkComponent まで api-index.md に公開 API として載っていた）
	if compoundName:match("^Tsukino::Sandbox") then
		return nil
	end

	local kind = source:match('<compounddef[^>]-kind="([%w]+)"') or "class"

	-- 定義元ヘッダ。モジュール名はそのパスの先頭から取る
	local headerFile = source:match('<location file="([^"]+)"')
	local moduleName = headerFile and headerFile:match("^([^/]+)") or "?"

	local functions = {}
	local variables = {}
	local enums     = {}

	for memberBlock in source:gmatch("<memberdef.-</memberdef>") do
		local prot = memberBlock:match('prot="([%w]+)"')
		if prot == "public" and memberBlock:match('^<memberdef%s+kind="enum"') then
			-- クラスの中の列挙型（CameraComponent::ProjectionType など）
			enums[#enums + 1] = ParseEnumBlock(memberBlock, DecodeXmlEntities(compoundName))
		elseif prot == "public" then
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
		enums     = enums,
	}
end

-- 名前空間の XML を1つ解析し、公開ヘッダにある列挙型・型の別名・関数・定数を集めます。
-- クラス／構造体は ParseCompoundXml が別に拾うので、ここでは名前空間の直下にあるものだけを扱う。
-- @param  [in] xmlPath XML ファイルのパス
-- @return { { kind, namespace, module, header, name, signature, brief, enum } } / 対象外なら {}
local function ParseNamespaceXml(xmlPath)
	local handle = io.open(xmlPath, "rb")
	if not handle then
		return {}
	end
	local source = handle:read("*a")
	handle:close()

	local namespaceName = source:match("<compoundname>([^<]+)</compoundname>")
	if not namespaceName then
		return {}
	end
	namespaceName = DecodeXmlEntities(namespaceName)
	if not (namespaceName == "Tsukino" or namespaceName:match("^Tsukino::")) or namespaceName:match("^Tsukino::Sandbox") then
		return {}
	end

	local items = {}
	for memberBlock in source:gmatch("<memberdef.-</memberdef>") do
		local memberKind = memberBlock:match('^<memberdef%s+kind="([%w-]+)"')

		-- 実装（.cpp）の中だけのものは公開 API ではない。宣言が公開ヘッダにあるものに限る
		local header = MemberDeclarationFile((memberKind == "enum") and memberBlock:gsub("<enumvalue.-</enumvalue>", "") or memberBlock)
		if header and header:match("^Tsukino%.[%w]+/include/") and not header:match("^Tsukino%.Sandbox") then
			local item = { kind = memberKind, namespace = namespaceName, module = header:match("^([^/]+)"), header = header }

			if memberKind == "enum" then
				item.enum      = ParseEnumBlock(memberBlock, namespaceName)
				item.name      = item.enum.qualifiedName
				item.brief     = item.enum.brief
				items[#items + 1] = item
			elseif memberKind == "function" or memberKind == "variable" or memberKind == "typedef" then
				-- テンプレート引数リストは <type> より前に現れ、その中にも <type> があるため先に切り離す（ParseCompoundXml と同じ）
				local templateBlock = memberBlock:match("<templateparamlist>(.-)</templateparamlist>")
				local plainBlock    = memberBlock:gsub("<templateparamlist>.-</templateparamlist>", "")

				local templatePrefix = ""
				if templateBlock then
					local templateParams = {}
					for paramBlock in templateBlock:gmatch("<param>(.-)</param>") do
						local paramType = FlattenXmlText(paramBlock:match("<type>(.-)</type>"))
						local paramName = paramBlock:match("<declname>([^<]*)</declname>")
						templateParams[#templateParams + 1] = paramName and (paramType .. " " .. DecodeXmlEntities(paramName)) or paramType
					end
					if #templateParams > 0 then
						templatePrefix = "template <" .. table.concat(templateParams, ", ") .. "> "
					end
				end

				local name        = DecodeXmlEntities(plainBlock:match("<name>([^<]*)</name>") or "?")
				local typeText    = FlattenXmlText(plainBlock:match("<type>(.-)</type>"))
				local args        = DecodeXmlEntities(plainBlock:match("<argsstring>([^<]*)</argsstring>") or "")
				local initializer = FlattenXmlText(plainBlock:match("<initializer>(.-)</initializer>"))
				local definition  = FlattenXmlText(plainBlock:match("<definition>(.-)</definition>"))

				item.name  = name
				item.brief = FlattenXmlText(plainBlock:match("<briefdescription>(.-)</briefdescription>"))
				if memberKind == "function" then
					item.signature = templatePrefix .. ((typeText ~= "") and (typeText .. " ") or "") .. name .. args
				elseif memberKind == "variable" then
					item.signature = ((typeText ~= "") and (typeText .. " ") or "") .. name .. ((initializer ~= "") and (" " .. initializer) or "")
				else
					-- 型の別名は definition が "using Tsukino::ECS::Entity = entt::entity" の形で入っている
					item.name      = namespaceName .. "::" .. name
					item.signature = (definition ~= "") and definition or ("using " .. name .. " = " .. typeText)
				end
				items[#items + 1] = item
			end
		end
	end
	return items
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

-- 名前空間の直下にあるもの（関数・定数）を、同じ名前空間・同じヘッダごとにまとめます。
-- @param  [in] items ParseNamespaceXml の結果のうち、同じ種類のもの
-- @return { { namespace, header, items } }（名前空間 → ヘッダの順）
local function GroupByNamespaceAndHeader(items)
	local groups = {}
	local keys   = {}
	for _, item in ipairs(items) do
		local key = item.namespace .. "|" .. item.header
		if not groups[key] then
			groups[key] = { namespace = item.namespace, header = item.header, items = {} }
			keys[#keys + 1] = key
		end
		local group = groups[key]
		group.items[#group.items + 1] = item
	end
	table.sort(keys)
	local result = {}
	for _, key in ipairs(keys) do
		result[#result + 1] = groups[key]
	end
	return result
end

-- 指定した種類のものだけを取り出します。
local function FilterKind(items, kind)
	local result = {}
	for _, item in ipairs(items) do
		if item.kind == kind then
			result[#result + 1] = item
		end
	end
	return result
end

-- モジュール1つぶんの列挙型（名前空間の直下とクラスの中）を、完全名の順に集めます。
local function CollectEnums(moduleEntries, moduleExtras)
	local enums = {}
	for _, entry in ipairs(moduleEntries) do
		for _, enum in ipairs(entry.enums or {}) do
			enums[#enums + 1] = enum
		end
	end
	for _, item in ipairs(FilterKind(moduleExtras, "enum")) do
		enums[#enums + 1] = item.enum
	end
	table.sort(enums, function(a, b) return a.qualifiedName < b.qualifiedName end)
	return enums
end

-- 重ならない名前の一覧（オーバーロードは1つにまとめる）を作ります。
local function UniqueNames(items, suffix)
	local seen  = {}
	local names = {}
	for _, item in ipairs(items) do
		if not seen[item.name] then
			seen[item.name] = true
			names[#names + 1] = item.name .. (suffix or "")
		end
	end
	return names
end

-- api-index.md（名前 → ヘッダ → モジュール文書 の1行索引）を組み立てます。
-- @param  [in] moduleOrder   モジュール名の並び
-- @param  [in] moduleEntries モジュール名 → クラス／構造体の一覧
-- @param  [in] moduleExtras  モジュール名 → 名前空間の直下にあるもの（列挙型・型の別名・関数・定数）の一覧
local function BuildIndex(moduleOrder, moduleEntries, moduleExtras, missingPrimaries)
	local lines = {}
	lines[#lines + 1] = "# API 索引"
	lines[#lines + 1] = ""
	lines[#lines + 1] = "**このファイルは自動生成です。直接編集しないでください。**"
	lines[#lines + 1] = ""
	lines[#lines + 1] = "再生成: `generate-docs.bat`（doxygen で XML を出してから gen-api-digest）"
	lines[#lines + 1] = ""
	lines[#lines + 1] = "エンジンのヘッダを直接読む前に、まずここで型・列挙型・型の別名・関数の在処を引く。"
	lines[#lines + 1] = "列挙型の値や関数のシグネチャが要るときだけ `api/<モジュール>.md` を1本だけ開く（全部は開かない）。"
	lines[#lines + 1] = "コンポーネントと Prefab JSON のフィールドは `components.md` を見る。"

	for _, moduleName in ipairs(moduleOrder) do
		local entries = moduleEntries[moduleName] or {}
		local extras  = moduleExtras[moduleName] or {}

		lines[#lines + 1] = ""
		lines[#lines + 1] = "## " .. moduleName .. " — [`api/" .. moduleName .. ".md`](api/" .. moduleName .. ".md)"
		lines[#lines + 1] = ""
		for _, entry in ipairs(entries) do
			lines[#lines + 1] = "- `" .. entry.name .. "` — `" .. entry.header .. "`"
		end
		for _, enum in ipairs(CollectEnums(entries, extras)) do
			lines[#lines + 1] = "- `" .. enum.qualifiedName .. "`（列挙型）— `" .. enum.header .. "`"
		end
		for _, alias in ipairs(FilterKind(extras, "typedef")) do
			lines[#lines + 1] = "- `" .. alias.name .. "`（型の別名）— `" .. alias.header .. "`"
		end
		for _, group in ipairs(GroupByNamespaceAndHeader(FilterKind(extras, "function"))) do
			lines[#lines + 1] = "- `" .. group.namespace .. "::` の関数 " .. table.concat(UniqueNames(group.items, "()"), ", ") .. " — `" .. group.header .. "`"
		end
		for _, group in ipairs(GroupByNamespaceAndHeader(FilterKind(extras, "variable"))) do
			lines[#lines + 1] = "- `" .. group.namespace .. "::` の定数 " .. table.concat(UniqueNames(group.items), ", ") .. " — `" .. group.header .. "`"
		end
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

-- モジュール1つぶんの api/<Module>.md を組み立てます。
local function BuildModuleDoc(moduleName, moduleEntries, moduleExtras, primaryNames)
	local lines = {}
	lines[#lines + 1] = "# " .. moduleName .. " の公開 API"
	lines[#lines + 1] = ""
	lines[#lines + 1] = "**このファイルは自動生成です。直接編集しないでください。**"
	lines[#lines + 1] = ""
	lines[#lines + 1] = "型の在処だけ知りたいときは `../api-index.md` を見る。"
	lines[#lines + 1] = ""

	local hasPrimary = false
	for _, entry in ipairs(moduleEntries) do
		if primaryNames[entry.shortName] then
			hasPrimary = true
		end
	end

	if hasPrimary then
		lines[#lines + 1] = "## 主要な型"
		lines[#lines + 1] = ""
		lines[#lines + 1] = "ゲーム側から実際に触る型。メンバを全て展開している。"
		lines[#lines + 1] = ""
		for _, entry in ipairs(moduleEntries) do
			if primaryNames[entry.shortName] then
				BuildPrimarySection(entry, lines)
			end
		end
	end

	lines[#lines + 1] = "## 全公開型の索引"
	lines[#lines + 1] = ""
	lines[#lines + 1] = "メンバ名のみ。詳細が要るときはヘッダを開く。"
	lines[#lines + 1] = ""

	for _, entry in ipairs(moduleEntries) do
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

	--------------------------------------------------------------------------
	-- 型以外の公開 API。値やシグネチャまで載せ、ヘッダを開かずに済むようにする
	--------------------------------------------------------------------------
	local enums = CollectEnums(moduleEntries, moduleExtras)
	if #enums > 0 then
		lines[#lines + 1] = ""
		lines[#lines + 1] = "## 列挙型"
		lines[#lines + 1] = ""
		for _, enum in ipairs(enums) do
			local brief = (enum.brief ~= "") and (" — " .. enum.brief) or ""
			lines[#lines + 1] = "- **" .. enum.qualifiedName .. "** — `" .. enum.header .. "`" .. brief
			for _, value in ipairs(enum.values) do
				local initializer = (value.initializer ~= "") and (" " .. value.initializer) or ""
				local valueBrief  = (value.brief ~= "") and (" — " .. value.brief) or ""
				lines[#lines + 1] = "  - `" .. value.name .. initializer .. "`" .. valueBrief
			end
		end
	end

	local aliases = FilterKind(moduleExtras, "typedef")
	if #aliases > 0 then
		lines[#lines + 1] = ""
		lines[#lines + 1] = "## 型の別名"
		lines[#lines + 1] = ""
		for _, alias in ipairs(aliases) do
			local brief = (alias.brief ~= "") and (" — " .. alias.brief) or ""
			lines[#lines + 1] = "- `" .. alias.signature .. "` — `" .. alias.header .. "`" .. brief
		end
	end

	local sections = {
		{ kind = "function", title = "名前空間の関数" },
		{ kind = "variable", title = "名前空間の定数・変数" },
	}
	for _, section in ipairs(sections) do
		local groups = GroupByNamespaceAndHeader(FilterKind(moduleExtras, section.kind))
		if #groups > 0 then
			lines[#lines + 1] = ""
			lines[#lines + 1] = "## " .. section.title
			for _, group in ipairs(groups) do
				lines[#lines + 1] = ""
				lines[#lines + 1] = "### " .. group.namespace .. " — `" .. group.header .. "`"
				lines[#lines + 1] = ""
				lines[#lines + 1] = "| シグネチャ | 説明 |"
				lines[#lines + 1] = "|---|---|"
				for _, item in ipairs(group.items) do
					lines[#lines + 1] = "| `" .. EscapeForTable(item.signature) .. "` | " .. EscapeForTable(item.brief) .. " |"
				end
			end
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
	description = "Doxygen の XML から Docs/api-index.md と Docs/api/*.md を生成する（先に doxygen を実行しておくこと）",

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
		local extras  = {}    -- 名前空間の直下にあるもの（列挙型・型の別名・関数・定数）
		for _, xmlPath in ipairs(os.matchfiles(path.join(xmlDir, "*.xml"))) do
			local fileName = path.getname(xmlPath)
			if fileName:match("^class") or fileName:match("^struct") then
				local entry = ParseCompoundXml(xmlPath)
				if entry then
					entries[#entries + 1] = entry
				end
			elseif fileName:match("^namespace") then
				for _, item in ipairs(ParseNamespaceXml(xmlPath)) do
					extras[#extras + 1] = item
				end
			end
		end

		-- 名前空間 → 名前 → ヘッダの順に固定し、生成結果を決定的にする
		table.sort(extras, function(a, b)
			if a.namespace ~= b.namespace then
				return a.namespace < b.namespace
			end
			if a.name ~= b.name then
				return a.name < b.name
			end
			return (a.signature or "") < (b.signature or "")
		end)

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
		--
		-- モジュール別に分ける。1ファイルにまとめると全文読み込みで数万トークンかかり、
		-- 「ヘッダを読ませない代わりに引かせる」という索引の目的を裏切るため。
		----------------------------------------------------------------------
		local docsDir = path.join(engineRoot, "Docs")
		local apiDir  = path.join(docsDir, "api")
		os.mkdir(apiDir)

		-- モジュールが減ったときに古い文書を残さない
		for _, stalePath in ipairs(os.matchfiles(path.join(apiDir, "*.md"))) do
			os.remove(stalePath)
		end

		local function WriteFile(outputPath, contents)
			local handle = io.open(outputPath, "wb")
			if not handle then
				error("gen-api-digest: 書き込みに失敗しました: " .. outputPath)
			end
			handle:write(contents)
			handle:close()
		end

		-- モジュールごとに振り分ける。型が1つも無く関数だけのモジュールもあり得るので、両方から集める
		local moduleBuckets = {}
		local moduleExtras  = {}
		local moduleSeen    = {}
		local moduleOrder   = {}
		local function NoteModule(moduleName)
			if not moduleSeen[moduleName] then
				moduleSeen[moduleName]       = true
				moduleOrder[#moduleOrder + 1] = moduleName
				moduleBuckets[moduleName]    = {}
				moduleExtras[moduleName]     = {}
			end
		end
		for _, entry in ipairs(entries) do
			NoteModule(entry.module)
			local bucket = moduleBuckets[entry.module]
			bucket[#bucket + 1] = entry
		end
		for _, item in ipairs(extras) do
			NoteModule(item.module)
			local bucket = moduleExtras[item.module]
			bucket[#bucket + 1] = item
		end
		table.sort(moduleOrder)

		WriteFile(path.join(docsDir, "api-index.md"), BuildIndex(moduleOrder, moduleBuckets, moduleExtras, missingPrimaries))

		for _, moduleName in ipairs(moduleOrder) do
			WriteFile(path.join(apiDir, moduleName .. ".md"), BuildModuleDoc(moduleName, moduleBuckets[moduleName], moduleExtras[moduleName], primaryNames))
		end

		-- 分割前の単一ファイル。残っていると古い情報を読ませてしまう
		os.remove(path.join(docsDir, "api-digest.md"))

		local enumCount = #FilterKind(extras, "enum")
		for _, entry in ipairs(entries) do
			enumCount = enumCount + #entry.enums
		end
		print(string.format("gen-api-digest: 公開型 %d 件 / 列挙型 %d 件 / 型の別名 %d 件 / 関数 %d 件 / 定数 %d 件 / モジュール %d 件",
			#entries, enumCount, #FilterKind(extras, "typedef"), #FilterKind(extras, "function"), #FilterKind(extras, "variable"), #moduleOrder))
		if #missingPrimaries > 0 then
			print("gen-api-digest: 主要な型のうち未検出: " .. table.concat(missingPrimaries, ", "))
		end
		print("gen-api-digest: Docs/api-index.md と Docs/api/*.md")
	end,
}
