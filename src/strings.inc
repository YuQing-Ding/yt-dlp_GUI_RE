#pragma once
// strings.inc —— 全部界面文案。
//
// 每条词条一行 X(...)，参数顺序固定为：
//     X(ID, 简体中文, English, 繁體中文, 日本語, 한국어)
//
// i18n.h 用它生成枚举，i18n.cpp 用它生成查表数组。两边出自同一份清单，
// 所以不可能出现"加了一条却忘了补某个语言"导致的错位。
//
// 带 % 占位符的词条，各语言的占位符个数和类型必须一致。

#define STRING_TABLE(X)                                                                   \
                                                                                          \
/* ---------------------------------------------------------------- 程序 / 卡片 */        \
/* 产品名，五种语言统一不翻译 —— 和 exe 文件名保持一致 */                                  \
X(S_APP_TITLE,                                                                            \
  L"yt-dlp GUI RE", L"yt-dlp GUI RE", L"yt-dlp GUI RE",                                   \
  L"yt-dlp GUI RE", L"yt-dlp GUI RE")                                                     \
X(S_CARD_LINKS,                                                                           \
  L"链接", L"Links", L"連結", L"リンク", L"링크")                                          \
X(S_CARD_OUTPUT,                                                                          \
  L"输出与画质", L"Output & Quality", L"輸出與畫質", L"出力と画質", L"출력 및 화질")        \
X(S_CARD_OPTIONS,                                                                         \
  L"选项", L"Options", L"選項", L"オプション", L"옵션")                                     \
X(S_CARD_LOG,                                                                             \
  L"日志", L"Log", L"日誌", L"ログ", L"로그")                                              \
                                                                                          \
/* ---------------------------------------------------------------- 链接卡片 */           \
X(S_URL_CUE,                                                                              \
  L"粘贴视频或播放列表链接，每行一个",                                                      \
  L"Paste video or playlist links, one per line",                                         \
  L"貼上影片或播放清單連結，一行一個",                                                      \
  L"動画または再生リストの URL を 1 行に 1 つずつ貼り付けてください",                       \
  L"동영상 또는 재생목록 링크를 한 줄에 하나씩 붙여넣으세요")                               \
X(S_BTN_PASTE,                                                                            \
  L"粘贴", L"Paste", L"貼上", L"貼り付け", L"붙여넣기")                                    \
X(S_BTN_CLEAR,                                                                            \
  L"清空", L"Clear", L"清除", L"クリア", L"지우기")                                        \
X(S_BTN_PROBE,                                                                            \
  L"探测可用格式…", L"List formats…", L"探測可用格式…",                                    \
  L"利用可能な形式を取得…", L"사용 가능한 형식 탐색…")                                      \
X(S_URL_COUNT,                                                                            \
  L"%zu 个链接", L"%zu links", L"%zu 個連結", L"%zu 件のリンク", L"링크 %zu개")             \
                                                                                          \
/* ---------------------------------------------------------------- 输出 */               \
X(S_LBL_SAVETO,                                                                           \
  L"保存到", L"Save to", L"儲存至", L"保存先", L"저장 위치")                               \
X(S_BTN_BROWSE,                                                                           \
  L"浏览…", L"Browse…", L"瀏覽…", L"参照…", L"찾아보기…")                                  \
X(S_BTN_OPEN,                                                                             \
  L"打开", L"Open", L"開啟", L"開く", L"열기")                                             \
X(S_LBL_FILENAME,                                                                         \
  L"文件命名", L"Filename", L"檔案命名", L"ファイル名", L"파일 이름")                       \
X(S_LBL_QUALITY,                                                                          \
  L"画质排序", L"Quality", L"畫質排序", L"画質の優先順位", L"화질 정렬")                    \
X(S_LBL_MAXRES,                                                                           \
  L"上限", L"Max", L"上限", L"上限", L"최대")                                              \
X(S_LBL_CONTAINER,                                                                        \
  L"容器", L"Format", L"容器", L"コンテナ", L"컨테이너")                                    \
X(S_CHK_RECODE,                                                                           \
  L"重编码", L"Re-encode", L"重新編碼", L"再エンコード", L"재인코딩")                       \
X(S_CHK_AUDIOONLY,                                                                        \
  L"仅音频", L"Audio only", L"僅音訊", L"音声のみ", L"오디오만")                            \
X(S_CHK_CUSTOMFMT,                                                                        \
  L"自定义格式", L"Custom format", L"自訂格式", L"カスタム形式", L"사용자 지정 형식")        \
X(S_CUSTOMFMT_CUE,                                                                        \
  L"直接写 -f 表达式，例如 137+251",                                                        \
  L"Raw -f expression, e.g. 137+251",                                                     \
  L"直接寫 -f 運算式，例如 137+251",                                                        \
  L"-f 式を直接入力（例: 137+251）",                                                        \
  L"-f 표현식 직접 입력 (예: 137+251)")                                                     \
                                                                                          \
/* ---------------------------------------------------------------- 画质排序预设 */       \
X(S_Q_BEST,                                                                               \
  L"最高画质 · 分辨率优先，同分辨率取最高码率",                                              \
  L"Best quality · resolution first, then highest bitrate",                               \
  L"最高畫質 · 解析度優先，同解析度取最高位元率",                                            \
  L"最高画質 · 解像度優先、同解像度ではビットレート最大",                                    \
  L"최고 화질 · 해상도 우선, 동일 해상도에서 최고 비트레이트")                               \
X(S_Q_BITRATE,                                                                            \
  L"最高码率 · 纯码率优先（未必是最高分辨率）",                                              \
  L"Highest bitrate only (may not be highest resolution)",                                \
  L"最高位元率 · 純位元率優先（未必是最高解析度）",                                          \
  L"ビットレート最優先（解像度は最高とは限りません）",                                       \
  L"비트레이트 우선 (최고 해상도가 아닐 수 있음)")                                           \
X(S_Q_FPS,                                                                                \
  L"最高帧率优先", L"Highest frame rate first", L"最高影格率優先",                          \
  L"フレームレート優先", L"프레임레이트 우선")                                              \
X(S_Q_HDR,                                                                                \
  L"HDR / 10bit 优先", L"Prefer HDR / 10-bit", L"HDR / 10bit 優先",                        \
  L"HDR / 10bit を優先", L"HDR / 10비트 우선")                                             \
X(S_Q_DEFAULT,                                                                            \
  L"yt-dlp 默认排序（偏好 AV1，省空间）",                                                   \
  L"yt-dlp default (prefers AV1, smaller files)",                                         \
  L"yt-dlp 預設排序（偏好 AV1，較省空間）",                                                 \
  L"yt-dlp 既定（AV1 優先・容量節約）",                                                     \
  L"yt-dlp 기본값 (AV1 선호, 용량 절약)")                                                   \
X(S_Q_SMALLEST,                                                                           \
  L"最小体积", L"Smallest file", L"最小檔案", L"最小サイズ", L"최소 용량")                  \
                                                                                          \
/* ---------------------------------------------------------------- 分辨率上限 */         \
X(S_RES_NONE,                                                                             \
  L"不限", L"No limit", L"不限", L"制限なし", L"제한 없음")                                 \
                                                                                          \
/* ---------------------------------------------------------------- 容器 */               \
X(S_CT_AUTO,                                                                              \
  L"自动 · 不转换", L"Auto · no conversion", L"自動 · 不轉換",                              \
  L"自動 · 変換しない", L"자동 · 변환 안 함")                                               \
X(S_CT_MKV,                                                                               \
  L"MKV · 万能容器", L"MKV · universal", L"MKV · 萬用容器",                                 \
  L"MKV · 汎用コンテナ", L"MKV · 범용 컨테이너")                                            \
                                                                                          \
/* ---------------------------------------------------------------- 音频格式 */           \
X(S_AF_BEST,                                                                              \
  L"最佳（保持原格式，不转码）",                                                            \
  L"Best (keep original, no transcode)",                                                  \
  L"最佳（保持原格式，不轉碼）",                                                            \
  L"最高（元の形式のまま・再変換なし）",                                                     \
  L"최고 (원본 유지, 변환 안 함)")                                                          \
X(S_AQ_FMT,                                                                               \
  L"音质 %d", L"Quality %d", L"音質 %d", L"音質 %d", L"음질 %d")                            \
X(S_AQ_BEST,                                                                              \
  L"（最好）", L" (best)", L"（最佳）", L"（最高）", L" (최고)")                            \
X(S_AQ_WORST,                                                                             \
  L"（最差）", L" (worst)", L"（最差）", L"（最低）", L" (최저)")                           \
                                                                                          \
/* ---------------------------------------------------------------- 命名模板 */           \
X(S_TM_TITLE_ID,                                                                          \
  L"标题 [ID].扩展名", L"Title [ID].ext", L"標題 [ID].副檔名",                              \
  L"タイトル [ID].拡張子", L"제목 [ID].확장자")                                             \
X(S_TM_TITLE,                                                                             \
  L"标题.扩展名", L"Title.ext", L"標題.副檔名", L"タイトル.拡張子", L"제목.확장자")          \
X(S_TM_UPLOADER,                                                                          \
  L"上传者 - 标题 [ID]", L"Uploader - Title [ID]", L"上傳者 - 標題 [ID]",                   \
  L"投稿者 - タイトル [ID]", L"업로더 - 제목 [ID]")                                         \
X(S_TM_DATE,                                                                              \
  L"上传日期 - 标题 [ID]", L"Upload date - Title [ID]", L"上傳日期 - 標題 [ID]",            \
  L"投稿日 - タイトル [ID]", L"업로드 날짜 - 제목 [ID]")                                    \
X(S_TM_INDEX,                                                                             \
  L"序号 - 标题 [ID]", L"Index - Title [ID]", L"序號 - 標題 [ID]",                          \
  L"連番 - タイトル [ID]", L"번호 - 제목 [ID]")                                             \
                                                                                          \
/* ---------------------------------------------------------------- 标签页 */             \
X(S_TAB_COOKIES,                                                                          \
  L"Cookies", L"Cookies", L"Cookies", L"Cookie", L"쿠키")                                  \
X(S_TAB_SUBS,                                                                             \
  L"字幕与嵌入", L"Subtitles & Metadata", L"字幕與嵌入",                                    \
  L"字幕とメタデータ", L"자막 및 메타데이터")                                                \
X(S_TAB_NETWORK,                                                                          \
  L"网络", L"Network", L"網路", L"ネットワーク", L"네트워크")                                \
X(S_TAB_PLAYLIST,                                                                         \
  L"播放列表", L"Playlist", L"播放清單", L"再生リスト", L"재생목록")                         \
X(S_TAB_ADVANCED,                                                                         \
  L"高级", L"Advanced", L"進階", L"詳細設定", L"고급")                                      \
                                                                                          \
/* ---------------------------------------------------------------- Cookies 页 */         \
X(S_CK_NONE,                                                                              \
  L"不使用 Cookies", L"No cookies", L"不使用 Cookies",                                      \
  L"Cookie を使わない", L"쿠키 사용 안 함")                                                 \
X(S_CK_FILE,                                                                              \
  L"Cookies 文件（cookies.txt）", L"Cookie file (cookies.txt)",                             \
  L"Cookies 檔案（cookies.txt）", L"Cookie ファイル（cookies.txt）",                        \
  L"쿠키 파일 (cookies.txt)")                                                               \
X(S_CK_BROWSER,                                                                           \
  L"从浏览器读取", L"Read from browser", L"從瀏覽器讀取",                                   \
  L"ブラウザーから読み込む", L"브라우저에서 가져오기")                                        \
X(S_CK_PICKFILE,                                                                          \
  L"选择文件…", L"Choose file…", L"選擇檔案…", L"ファイルを選択…", L"파일 선택…")            \
X(S_CK_PROFILE_CUE,                                                                       \
  L"配置文件名，留空 = 最近使用的",                                                          \
  L"Profile name, blank = most recently used",                                            \
  L"設定檔名稱，留空 = 最近使用的",                                                          \
  L"プロファイル名（空欄なら最後に使ったもの）",                                             \
  L"프로필 이름, 비워두면 최근 사용 프로필")                                                 \
X(S_CK_FIREFOX_NOTE,                                                                      \
  L"Firefox（Windows 上最可靠）", L"Firefox (most reliable on Windows)",                   \
  L"Firefox（Windows 上最可靠）", L"Firefox（Windows で最も確実）",                         \
  L"Firefox (Windows에서 가장 안정적)")                                                     \
X(S_CK_HINT,                                                                              \
  L"Cookie 会被 YouTube 轮换。想长期有效：用一个专用浏览器配置登录后再也不打开它，"           \
  L"或用无痕窗口登录后导出 cookies.txt 并立刻关掉窗口。",                                    \
  L"YouTube rotates cookies. To keep them working: sign in using a dedicated browser "     \
  L"profile you never open again, or export cookies.txt from a private window and "        \
  L"close that window immediately.",                                                      \
  L"Cookie 會被 YouTube 輪換。想長期有效：用一個專用瀏覽器設定檔登入後再也不開啟它，"         \
  L"或用私密視窗登入後匯出 cookies.txt 並立刻關掉視窗。",                                    \
  L"YouTube は Cookie を定期的に更新します。長く使うには、専用のブラウザープロファイルで"    \
  L"ログインして以後そのプロファイルを開かないか、プライベートウィンドウで "                 \
  L"cookies.txt を書き出した直後にそのウィンドウを閉じてください。",                         \
  L"YouTube는 쿠키를 주기적으로 교체합니다. 오래 쓰려면 전용 브라우저 프로필로 "             \
  L"로그인한 뒤 다시 열지 않거나, 시크릿 창에서 cookies.txt를 내보낸 직후 "                  \
  L"그 창을 닫으세요.")                                                                     \
                                                                                          \
/* ---------------------------------------------------------------- 字幕页 */             \
X(S_SUB_WRITE,                                                                            \
  L"下载字幕", L"Download subtitles", L"下載字幕", L"字幕をダウンロード", L"자막 다운로드")  \
X(S_SUB_AUTO,                                                                             \
  L"含自动生成字幕", L"Include auto-generated", L"含自動產生字幕",                          \
  L"自動生成字幕も含む", L"자동 생성 자막 포함")                                             \
X(S_SUB_EMBED,                                                                            \
  L"嵌入视频", L"Embed in video", L"嵌入影片", L"動画に埋め込む", L"영상에 포함")            \
X(S_SUB_SRT,                                                                              \
  L"转成 SRT", L"Convert to SRT", L"轉成 SRT", L"SRT に変換", L"SRT로 변환")                \
X(S_SUB_LANGS,                                                                            \
  L"字幕语言", L"Languages", L"字幕語言", L"字幕の言語", L"자막 언어")                       \
X(S_EMB_THUMB,                                                                            \
  L"嵌入封面", L"Embed cover art", L"嵌入封面", L"サムネイルを埋め込む", L"썸네일 포함")     \
X(S_EMB_META,                                                                             \
  L"嵌入元数据", L"Embed metadata", L"嵌入中繼資料",                                        \
  L"メタデータを埋め込む", L"메타데이터 포함")                                               \
X(S_EMB_CHAP,                                                                             \
  L"嵌入章节", L"Embed chapters", L"嵌入章節", L"チャプターを埋め込む", L"챕터 포함")        \
X(S_SPONSOR,                                                                              \
  L"SponsorBlock 去广告段", L"SponsorBlock: cut sponsor segments",                         \
  L"SponsorBlock 去廣告段", L"SponsorBlock で宣伝部分を除去",                               \
  L"SponsorBlock으로 광고 구간 제거")                                                       \
                                                                                          \
/* ---------------------------------------------------------------- 网络页 */             \
X(S_NET_PROXY,                                                                            \
  L"代理", L"Proxy", L"代理伺服器", L"プロキシ", L"프록시")                                 \
X(S_NET_PROXY_CUE,                                                                        \
  L"http://127.0.0.1:7890 或 socks5://…", L"http://127.0.0.1:7890 or socks5://…",          \
  L"http://127.0.0.1:7890 或 socks5://…", L"http://127.0.0.1:7890 または socks5://…",       \
  L"http://127.0.0.1:7890 또는 socks5://…")                                                \
X(S_NET_RATE,                                                                             \
  L"限速", L"Rate limit", L"限速", L"速度制限", L"속도 제한")                               \
X(S_NET_RATE_CUE,                                                                         \
  L"例如 5M", L"e.g. 5M", L"例如 5M", L"例: 5M", L"예: 5M")                                 \
X(S_NET_FRAGS,                                                                            \
  L"并发分片", L"Concurrency", L"並行分段", L"同時ダウンロード数", L"동시 조각 수")          \
X(S_NET_RETRIES,                                                                          \
  L"重试次数", L"Retries", L"重試次數", L"リトライ回数", L"재시도 횟수")                     \
X(S_NET_POLITE,                                                                           \
  L"温和模式（拉长请求间隔，降低被判定为机器人的概率）",                                      \
  L"Polite mode (slower request pacing, avoids bot detection)",                            \
  L"溫和模式（拉長請求間隔，降低被判定為機器人的機率）",                                      \
  L"低速モード（リクエスト間隔を空けて Bot 判定を避ける）",                                   \
  L"완만 모드 (요청 간격을 늘려 봇 감지를 회피)")                                            \
                                                                                          \
/* ---------------------------------------------------------------- 播放列表页 */         \
X(S_PL_RANGE,                                                                             \
  L"下载范围", L"Range", L"下載範圍", L"ダウンロード範囲", L"다운로드 범위")                 \
X(S_PL_RANGE_CUE,                                                                         \
  L"留空 = 全部；也可写 1-10 或 3,7,12-20",                                                 \
  L"Blank = all; or 1-10, or 3,7,12-20",                                                  \
  L"留空 = 全部；也可寫 1-10 或 3,7,12-20",                                                 \
  L"空欄ですべて。1-10 や 3,7,12-20 のような指定も可",                                       \
  L"비워두면 전체. 1-10 또는 3,7,12-20 형식도 가능")                                         \
X(S_PL_FOLDER,                                                                            \
  L"按播放列表名建子文件夹", L"Subfolder per playlist", L"依播放清單名稱建立子資料夾",        \
  L"再生リストごとにフォルダーを作成", L"재생목록별 하위 폴더 생성")                          \
X(S_PL_ARCHIVE,                                                                           \
  L"记录已下载，跳过重复", L"Remember downloads, skip duplicates",                          \
  L"記錄已下載，略過重複", L"ダウンロード済みを記録して重複を省く",                           \
  L"다운로드 기록을 남겨 중복 건너뛰기")                                                     \
X(S_PL_IGNERR,                                                                            \
  L"出错继续下一个", L"Continue past errors", L"出錯繼續下一個",                             \
  L"エラーが出ても次へ進む", L"오류가 나도 다음으로 계속")                                    \
X(S_PL_NOOVER,                                                                            \
  L"不覆盖已存在的文件", L"Never overwrite existing files", L"不覆寫已存在的檔案",           \
  L"既存ファイルを上書きしない", L"기존 파일 덮어쓰지 않음")                                  \
                                                                                          \
/* ---------------------------------------------------------------- 高级页 */             \
X(S_ADV_EXTRA,                                                                            \
  L"额外参数", L"Extra args", L"額外參數", L"追加オプション", L"추가 인수")                  \
X(S_ADV_EXTRA_CUE,                                                                        \
  L"原样追加到 yt-dlp 命令行，例如 --write-description",                                    \
  L"Appended verbatim to the yt-dlp command, e.g. --write-description",                   \
  L"原樣附加到 yt-dlp 命令列，例如 --write-description",                                    \
  L"yt-dlp のコマンドラインにそのまま追加（例: --write-description）",                       \
  L"yt-dlp 명령줄에 그대로 추가 (예: --write-description)")                                 \
X(S_ADV_DETECTING,                                                                        \
  L"检测中…", L"Detecting…", L"偵測中…", L"検出中…", L"확인 중…")                            \
X(S_ADV_GET,                                                                              \
  L"下载 yt-dlp (x64)", L"Get yt-dlp (x64)", L"下載 yt-dlp (x64)",                          \
  L"yt-dlp を入手 (x64)", L"yt-dlp 받기 (x64)")                                            \
X(S_ADV_UPDATE,                                                                           \
  L"更新", L"Update", L"更新", L"更新", L"업데이트")                                        \
X(S_ADV_SHOWCMD,                                                                          \
  L"预览命令行", L"Preview command", L"預覽命令列",                                         \
  L"コマンドを確認", L"명령어 미리보기")                                                    \
X(S_ADV_NOTFOUND,                                                                         \
  L"未找到可用的 yt-dlp", L"No working yt-dlp found", L"找不到可用的 yt-dlp",               \
  L"使用可能な yt-dlp が見つかりません", L"사용 가능한 yt-dlp를 찾을 수 없음")                \
                                                                                          \
/* ---------------------------------------------------------------- 运行 / 日志 */        \
X(S_BTN_START,                                                                            \
  L"开始下载", L"Download", L"開始下載", L"ダウンロード開始", L"다운로드 시작")              \
X(S_BTN_STOP,                                                                             \
  L"停止", L"Stop", L"停止", L"停止", L"중지")                                              \
X(S_CHK_AUTOSCROLL,                                                                       \
  L"自动滚动", L"Auto-scroll", L"自動捲動", L"自動スクロール", L"자동 스크롤")               \
X(S_BTN_COPY,                                                                             \
  L"复制", L"Copy", L"複製", L"コピー", L"복사")                                            \
X(S_BTN_SAVE,                                                                             \
  L"保存…", L"Save…", L"儲存…", L"保存…", L"저장…")                                         \
                                                                                          \
/* ---------------------------------------------------------------- 顶栏状态 */           \
X(S_HDR_YTDLP_NOTREADY,                                                                   \
  L"yt-dlp 未就绪", L"yt-dlp not ready", L"yt-dlp 未就緒",                                 \
  L"yt-dlp 未準備", L"yt-dlp 준비 안 됨")                                                   \
X(S_HDR_DENO_OK,                                                                          \
  L"Deno 已就绪", L"Deno ready", L"Deno 已就緒", L"Deno 利用可", L"Deno 준비됨")            \
X(S_HDR_DENO_MISSING,                                                                     \
  L"缺 Deno", L"Deno missing", L"缺 Deno", L"Deno なし", L"Deno 없음")                     \
X(S_HDR_FFMPEG_OK,                                                                        \
  L"ffmpeg 已就绪", L"ffmpeg ready", L"ffmpeg 已就緒", L"ffmpeg 利用可", L"ffmpeg 준비됨")  \
X(S_HDR_FFMPEG_MISSING,                                                                   \
  L"缺 ffmpeg", L"ffmpeg missing", L"缺 ffmpeg", L"ffmpeg なし", L"ffmpeg 없음")           \
                                                                                          \
/* ---------------------------------------------------------------- 状态 / 日志消息 */    \
X(S_LOG_STARTUP,                                                                          \
  L"就绪。正在检测 yt-dlp / Deno / ffmpeg …",                                              \
  L"Ready. Checking for yt-dlp / Deno / ffmpeg…",                                         \
  L"就緒。正在偵測 yt-dlp / Deno / ffmpeg …",                                              \
  L"準備完了。yt-dlp / Deno / ffmpeg を確認しています…",                                    \
  L"준비 완료. yt-dlp / Deno / ffmpeg 확인 중…")                                            \
X(S_LOG_NO_YTDLP,                                                                         \
  L"没有找到能正常运行的 yt-dlp。",                                                         \
  L"No working yt-dlp was found.",                                                        \
  L"找不到能正常執行的 yt-dlp。",                                                           \
  L"正常に動作する yt-dlp が見つかりませんでした。",                                        \
  L"정상 동작하는 yt-dlp를 찾지 못했습니다.")                                                \
X(S_LOG_NO_YTDLP_HINT,                                                                    \
  L"到「高级」标签页点「下载 yt-dlp (x64)」即可自动获取。",                                  \
  L"Open the Advanced tab and click \"Get yt-dlp (x64)\" to fetch it automatically.",      \
  L"到「進階」標籤頁點「下載 yt-dlp (x64)」即可自動取得。",                                  \
  L"「詳細設定」タブの「yt-dlp を入手 (x64)」で自動的に取得できます。",                      \
  L"'고급' 탭에서 'yt-dlp 받기 (x64)'를 누르면 자동으로 받을 수 있습니다.")                  \
X(S_LOG_DENO_WARN1,                                                                       \
  L"没有找到 Deno。YouTube 的签名和 n 参数需要一个 JS 运行时才能解算，",                     \
  L"Deno was not found. YouTube's signature and n-parameter need a JS runtime to solve,",  \
  L"找不到 Deno。YouTube 的簽章和 n 參數需要一個 JS 執行環境才能解算，",                     \
  L"Deno が見つかりません。YouTube の署名と n パラメーターの解決には JS ランタイムが必要で、",\
  L"Deno를 찾지 못했습니다. YouTube의 서명과 n 파라미터를 풀려면 JS 런타임이 필요하며,")     \
X(S_LOG_DENO_WARN2,                                                                       \
  L"否则会报 \"Signature solving failed\"，只能拿到低清或被限速的流。",                      \
  L"otherwise you get \"Signature solving failed\" and only low-quality or throttled streams.",\
  L"否則會出現 \"Signature solving failed\"，只能拿到低畫質或被限速的串流。",                 \
  L"ないと \"Signature solving failed\" となり、低画質か速度制限された映像しか取得できません。",\
  L"없으면 \"Signature solving failed\"가 발생하고 저화질 또는 속도 제한된 스트림만 받게 됩니다.")\
X(S_LOG_DENO_INSTALL,                                                                     \
  L"安装方法（PowerShell）：irm https://deno.land/install.ps1 | iex",                       \
  L"Install (PowerShell): irm https://deno.land/install.ps1 | iex",                       \
  L"安裝方法（PowerShell）：irm https://deno.land/install.ps1 | iex",                       \
  L"インストール（PowerShell）: irm https://deno.land/install.ps1 | iex",                   \
  L"설치 (PowerShell): irm https://deno.land/install.ps1 | iex")                           \
X(S_LOG_FFMPEG_WARN,                                                                      \
  L"没有找到 ffmpeg。视频和音频是分开的两条流，没有它就无法合并。",                          \
  L"ffmpeg was not found. Video and audio come as separate streams and cannot be merged without it.",\
  L"找不到 ffmpeg。影片和音訊是分開的兩條串流，沒有它就無法合併。",                          \
  L"ffmpeg が見つかりません。映像と音声は別ストリームなので、これがないと結合できません。",   \
  L"ffmpeg를 찾지 못했습니다. 영상과 오디오는 별도 스트림이라 이것 없이는 병합할 수 없습니다.")\
X(S_STATUS_RESOLVING,                                                                     \
  L"正在解析…", L"Resolving…", L"正在解析…", L"解析中…", L"분석 중…")                        \
X(S_STATUS_PROBING,                                                                       \
  L"正在探测可用格式…", L"Listing available formats…", L"正在探測可用格式…",                 \
  L"利用可能な形式を取得中…", L"사용 가능한 형식 확인 중…")                                  \
X(S_STATUS_STOPPING,                                                                      \
  L"正在停止…", L"Stopping…", L"正在停止…", L"停止しています…", L"중지하는 중…")             \
X(S_STATUS_STOPPED,                                                                       \
  L"已停止", L"Stopped", L"已停止", L"停止しました", L"중지됨")                             \
X(S_STATUS_DONE,                                                                          \
  L"完成", L"Done", L"完成", L"完了", L"완료")                                              \
X(S_STATUS_ERRORS,                                                                        \
  L"结束，有 %d 处错误", L"Finished with %d error(s)", L"結束，有 %d 處錯誤",                \
  L"%d 件のエラーで終了", L"오류 %d건으로 종료")                                            \
X(S_STATUS_FAILEDN,                                                                       \
  L"   ·  %d 个失败", L"   ·  %d failed", L"   ·  %d 個失敗",                               \
  L"   ·  %d 件失敗", L"   ·  %d개 실패")                                                   \
X(S_STATUS_ETA,                                                                           \
  L"剩余 ", L"ETA ", L"剩餘 ", L"残り ", L"남음 ")                                          \
X(S_LOG_STOPPED,                                                                          \
  L"\n—— 已手动停止 ——", L"\n—— Stopped by user ——", L"\n—— 已手動停止 ——",                  \
  L"\n—— ユーザーによって停止されました ——", L"\n—— 사용자가 중지함 ——")                     \
X(S_LOG_ALLDONE,                                                                          \
  L"\n—— 全部完成 ——", L"\n—— All done ——", L"\n—— 全部完成 ——",                             \
  L"\n—— すべて完了 ——", L"\n—— 모두 완료 ——")                                              \
X(S_LOG_EXITCODE,                                                                         \
  L"\n—— 结束，退出码 %lu（上面标红的行是原因）——",                                          \
  L"\n—— Finished with exit code %lu (see the red lines above) ——",                        \
  L"\n—— 結束，結束碼 %lu（上面標紅的行是原因）——",                                          \
  L"\n—— 終了コード %lu で終了（上の赤い行が原因です）——",                                   \
  L"\n—— 종료 코드 %lu (위의 빨간 줄이 원인입니다) ——")                                      \
X(S_LOG_NOSTART,                                                                          \
  L"无法启动 yt-dlp 进程。", L"Could not start the yt-dlp process.",                        \
  L"無法啟動 yt-dlp 處理程序。", L"yt-dlp プロセスを起動できませんでした。",                 \
  L"yt-dlp 프로세스를 시작할 수 없습니다.")                                                  \
X(S_LOG_PROBING,                                                                          \
  L"探测格式：", L"Listing formats for: ", L"探測格式：",                                   \
  L"形式を取得: ", L"형식 탐색: ")                                                          \
X(S_LOG_PROBE_FAILED,                                                                     \
  L"探测失败：", L"Format listing failed: ", L"探測失敗：",                                 \
  L"形式の取得に失敗: ", L"형식 탐색 실패: ")                                                \
X(S_LOG_PROBE_OK,                                                                         \
  L"探测到 %zu 个可用格式：%s", L"Found %zu formats: %s", L"探測到 %zu 個可用格式：%s",      \
  L"%zu 件の形式が見つかりました: %s", L"형식 %zu개를 찾았습니다: %s")                       \
X(S_LOG_FMT_CHOSEN,                                                                       \
  L"已选择格式：", L"Selected format: ", L"已選擇格式：",                                   \
  L"選択した形式: ", L"선택한 형식: ")                                                      \
X(S_LOG_UPDATING,                                                                         \
  L"正在更新 yt-dlp …", L"Updating yt-dlp…", L"正在更新 yt-dlp …",                          \
  L"yt-dlp を更新しています…", L"yt-dlp 업데이트 중…")                                       \
X(S_LOG_NOUPDATE,                                                                         \
  L"无法启动更新进程。", L"Could not start the updater.", L"無法啟動更新處理程序。",         \
  L"更新プロセスを起動できませんでした。", L"업데이터를 시작할 수 없습니다.")                 \
X(S_LOG_GETTING,                                                                          \
  L"开始下载 yt-dlp.exe …", L"Downloading yt-dlp.exe…", L"開始下載 yt-dlp.exe …",           \
  L"yt-dlp.exe をダウンロードしています…", L"yt-dlp.exe 다운로드 중…")                       \
X(S_STATUS_GETTING,                                                                       \
  L"正在从 GitHub 下载 yt-dlp…", L"Downloading yt-dlp from GitHub…",                       \
  L"正在從 GitHub 下載 yt-dlp…", L"GitHub から yt-dlp をダウンロード中…",                   \
  L"GitHub에서 yt-dlp 다운로드 중…")                                                        \
X(S_LOG_GETOK,                                                                            \
  L"yt-dlp.exe 下载完成，正在重新检测…",                                                    \
  L"yt-dlp.exe downloaded. Re-checking…",                                                 \
  L"yt-dlp.exe 下載完成，正在重新偵測…",                                                    \
  L"yt-dlp.exe のダウンロードが完了しました。再確認しています…",                             \
  L"yt-dlp.exe 다운로드 완료. 다시 확인 중…")                                                \
X(S_LOG_GETFAIL,                                                                          \
  L"下载失败。检查网络，或手动去 github.com/yt-dlp/yt-dlp/releases 下载。",                  \
  L"Download failed. Check your connection, or get it manually from github.com/yt-dlp/yt-dlp/releases",\
  L"下載失敗。請檢查網路，或手動到 github.com/yt-dlp/yt-dlp/releases 下載。",                \
  L"ダウンロードに失敗しました。接続を確認するか、github.com/yt-dlp/yt-dlp/releases から手動で入手してください。",\
  L"다운로드에 실패했습니다. 연결을 확인하거나 github.com/yt-dlp/yt-dlp/releases에서 직접 받으세요.")\
X(S_STATUS_GETFAIL,                                                                       \
  L"下载失败", L"Download failed", L"下載失敗", L"ダウンロード失敗", L"다운로드 실패")       \
X(S_LOG_LOGSAVED,                                                                         \
  L"日志已保存到 ", L"Log saved to ", L"日誌已儲存到 ",                                     \
  L"ログを保存しました: ", L"로그를 저장했습니다: ")                                         \
                                                                                          \
/* ---------------------------------------------------------------- 对话框 */             \
X(S_MSG_NOYTDLP,                                                                          \
  L"没找到可用的 yt-dlp。\n\n请把 yt-dlp.exe 放到本程序同一个文件夹里，\n"                   \
  L"或者到「高级」标签页点「下载 yt-dlp (x64)」自动获取。",                                  \
  L"No working yt-dlp found.\n\nPut yt-dlp.exe in the same folder as this program,\n"      \
  L"or open the Advanced tab and click \"Get yt-dlp (x64)\".",                             \
  L"找不到可用的 yt-dlp。\n\n請把 yt-dlp.exe 放到本程式同一個資料夾裡，\n"                   \
  L"或者到「進階」標籤頁點「下載 yt-dlp (x64)」自動取得。",                                  \
  L"使用可能な yt-dlp が見つかりません。\n\nyt-dlp.exe をこのプログラムと同じフォルダーに"   \
  L"置くか、\n「詳細設定」タブの「yt-dlp を入手 (x64)」を使ってください。",                  \
  L"사용 가능한 yt-dlp가 없습니다.\n\nyt-dlp.exe를 이 프로그램과 같은 폴더에 두거나,\n"      \
  L"'고급' 탭에서 'yt-dlp 받기 (x64)'를 누르세요.")                                         \
X(S_MSG_NOURL,                                                                            \
  L"请先在上面粘贴至少一个视频或播放列表链接。",                                             \
  L"Paste at least one video or playlist link first.",                                    \
  L"請先在上面貼上至少一個影片或播放清單連結。",                                             \
  L"動画または再生リストの URL を少なくとも 1 つ貼り付けてください。",                       \
  L"먼저 동영상 또는 재생목록 링크를 하나 이상 붙여넣으세요.")                               \
X(S_MSG_NOURL_PROBE,                                                                      \
  L"请先粘贴一个链接，再点探测。",                                                          \
  L"Paste a link first, then list formats.",                                              \
  L"請先貼上一個連結，再點探測。",                                                          \
  L"先に URL を貼り付けてから形式を取得してください。",                                     \
  L"먼저 링크를 붙여넣은 뒤 형식을 탐색하세요.")                                             \
X(S_MSG_NODIR,                                                                            \
  L"请先选择保存目录。", L"Choose an output folder first.", L"請先選擇儲存資料夾。",         \
  L"保存先フォルダーを選択してください。", L"저장 폴더를 먼저 선택하세요.")                   \
X(S_MSG_MKDIRFAIL,                                                                        \
  L"保存目录无法创建：\n", L"Could not create the output folder:\n",                        \
  L"儲存資料夾無法建立：\n", L"保存先フォルダーを作成できませんでした:\n",                   \
  L"저장 폴더를 만들 수 없습니다:\n")                                                        \
X(S_MSG_NOCOOKIE,                                                                         \
  L"Cookies 模式选了「文件」，但这个文件不存在。",                                           \
  L"Cookie mode is set to \"file\", but that file does not exist.",                        \
  L"Cookies 模式選了「檔案」，但這個檔案不存在。",                                           \
  L"Cookie の取得方法が「ファイル」ですが、そのファイルが存在しません。",                    \
  L"쿠키 방식이 '파일'로 설정되어 있지만 해당 파일이 없습니다.")                              \
X(S_MSG_QUIT,                                                                             \
  L"下载还没结束，确定要退出吗？", L"A download is still running. Quit anyway?",             \
  L"下載還沒結束，確定要離開嗎？", L"ダウンロードが進行中です。終了しますか？",               \
  L"다운로드가 진행 중입니다. 종료할까요?")                                                  \
X(S_MSG_GETCONFIRM,                                                                       \
  L"将从 GitHub 官方发布页下载最新的 yt-dlp.exe（x64 单文件版，约 17 MB），\n"               \
  L"保存到本程序所在文件夹，覆盖同名文件。\n\n继续吗？",                                     \
  L"This will download the latest yt-dlp.exe (x64 single-file build, about 17 MB)\n"       \
  L"from the official GitHub releases page into this program's folder,\n"                  \
  L"replacing any existing copy.\n\nContinue?",                                            \
  L"將從 GitHub 官方發布頁下載最新的 yt-dlp.exe（x64 單檔版，約 17 MB），\n"                 \
  L"儲存到本程式所在資料夾，覆寫同名檔案。\n\n要繼續嗎？",                                   \
  L"GitHub の公式リリースページから最新の yt-dlp.exe（x64 単一ファイル版・約 17 MB）を\n"    \
  L"このプログラムのフォルダーにダウンロードし、既存のファイルを置き換えます。\n\n続けますか？",\
  L"GitHub 공식 릴리스 페이지에서 최신 yt-dlp.exe(x64 단일 파일, 약 17 MB)를\n"             \
  L"이 프로그램 폴더에 내려받아 기존 파일을 덮어씁니다.\n\n계속할까요?")                      \
X(S_MSG_WRITEFAIL,                                                                        \
  L"写入失败。", L"Could not write the file.", L"寫入失敗。",                               \
  L"ファイルを書き込めませんでした。", L"파일을 쓸 수 없습니다.")                             \
X(S_MSG_PROBEFAIL,                                                                        \
  L"探测失败。\n\n", L"Could not list formats.\n\n", L"探測失敗。\n\n",                     \
  L"形式を取得できませんでした。\n\n", L"형식을 가져오지 못했습니다.\n\n")                   \
                                                                                          \
/* ---------------------------------------------------------------- 文件对话框过滤器 */   \
X(S_FILTER_TXT,                                                                           \
  L"文本文件 (*.txt)", L"Text files (*.txt)", L"文字檔 (*.txt)",                            \
  L"テキスト ファイル (*.txt)", L"텍스트 파일 (*.txt)")                                     \
X(S_FILTER_COOKIES,                                                                       \
  L"Cookies 文本 (*.txt)", L"Cookie files (*.txt)", L"Cookies 文字檔 (*.txt)",              \
  L"Cookie ファイル (*.txt)", L"쿠키 파일 (*.txt)")                                         \
X(S_FILTER_ALL,                                                                           \
  L"所有文件 (*.*)", L"All files (*.*)", L"所有檔案 (*.*)",                                 \
  L"すべてのファイル (*.*)", L"모든 파일 (*.*)")                                            \
                                                                                          \
/* ---------------------------------------------------------------- 格式选择窗 */         \
X(S_FP_TITLE,                                                                             \
  L"选择格式", L"Choose a format", L"選擇格式", L"形式を選択", L"형식 선택")                 \
X(S_FP_HINT,                                                                              \
  L"双击一行即可选中该格式。纯视频流会自动配上最佳音轨。",                                   \
  L"Double-click a row to pick it. Video-only streams get the best audio track added.",    \
  L"連按兩下一列即可選取該格式。純視訊串流會自動配上最佳音軌。",                             \
  L"行をダブルクリックで選択します。映像のみの形式には最適な音声が自動で追加されます。",      \
  L"행을 두 번 클릭하면 선택됩니다. 영상 전용 스트림에는 최적 오디오가 자동으로 추가됩니다.")  \
X(S_FP_DURATION,                                                                          \
  L"时长 ", L"Duration ", L"長度 ", L"再生時間 ", L"길이 ")                                 \
X(S_FP_UPLOADER,                                                                          \
  L"上传者 ", L"Uploader ", L"上傳者 ", L"投稿者 ", L"업로더 ")                             \
X(S_FP_COL_ID,                                                                            \
  L"格式 ID", L"Format ID", L"格式 ID", L"形式 ID", L"형식 ID")                             \
X(S_FP_COL_RES,                                                                           \
  L"分辨率", L"Resolution", L"解析度", L"解像度", L"해상도")                                \
X(S_FP_COL_FPS,                                                                           \
  L"FPS", L"FPS", L"FPS", L"FPS", L"FPS")                                                  \
X(S_FP_COL_VCODEC,                                                                        \
  L"视频编码", L"Video codec", L"視訊編碼", L"映像コーデック", L"영상 코덱")                 \
X(S_FP_COL_ACODEC,                                                                        \
  L"音频编码", L"Audio codec", L"音訊編碼", L"音声コーデック", L"오디오 코덱")               \
X(S_FP_COL_BITRATE,                                                                       \
  L"码率", L"Bitrate", L"位元率", L"ビットレート", L"비트레이트")                            \
X(S_FP_COL_SIZE,                                                                          \
  L"体积", L"Size", L"檔案大小", L"サイズ", L"크기")                                        \
X(S_FP_COL_EXT,                                                                           \
  L"容器", L"Container", L"容器", L"コンテナ", L"컨테이너")                                 \
X(S_FP_COL_NOTE,                                                                          \
  L"说明", L"Notes", L"說明", L"備考", L"비고")                                             \
X(S_FP_AUDIOONLY,                                                                         \
  L"仅音频", L"audio only", L"僅音訊", L"音声のみ", L"오디오만")                             \
X(S_FP_NOAUDIO,                                                                           \
  L"（无音轨）", L" (no audio)", L"（無音軌）", L"（音声なし）", L" (오디오 없음)")           \
X(S_FP_AUTOAUDIO,                                                                         \
  L"纯视频流自动合并最佳音频（推荐）",                                                       \
  L"Add the best audio track to video-only streams (recommended)",                        \
  L"純視訊串流自動合併最佳音訊（建議）",                                                     \
  L"映像のみの形式に最適な音声を自動で結合する（推奨）",                                     \
  L"영상 전용 스트림에 최적 오디오를 자동 병합 (권장)")                                      \
X(S_FP_USE,                                                                               \
  L"使用此格式", L"Use this format", L"使用此格式", L"この形式を使う", L"이 형식 사용")      \
X(S_FP_CANCEL,                                                                            \
  L"取消", L"Cancel", L"取消", L"キャンセル", L"취소")                                      \
X(S_FP_ERR_JSON,                                                                          \
  L"yt-dlp 返回的不是合法 JSON", L"yt-dlp did not return valid JSON",                      \
  L"yt-dlp 傳回的不是合法 JSON", L"yt-dlp が有効な JSON を返しませんでした",                 \
  L"yt-dlp가 올바른 JSON을 반환하지 않았습니다")                                             \
X(S_FP_ERR_EMPTYPL,                                                                       \
  L"播放列表里没有可用条目", L"The playlist has no usable entries",                         \
  L"播放清單裡沒有可用項目", L"再生リストに利用できる項目がありません",                       \
  L"재생목록에 사용 가능한 항목이 없습니다")                                                 \
X(S_FP_ERR_NOFMT,                                                                         \
  L"这个链接没有返回任何格式信息", L"This link returned no format information",             \
  L"這個連結沒有傳回任何格式資訊", L"この URL からは形式情報が取得できませんでした",          \
  L"이 링크에서 형식 정보를 가져오지 못했습니다")                                            \
X(S_FP_ERR_NOPARSE,                                                                       \
  L"没有解析到可下载的格式", L"No downloadable formats were found",                         \
  L"沒有解析到可下載的格式", L"ダウンロード可能な形式が見つかりませんでした",                 \
  L"다운로드 가능한 형식을 찾지 못했습니다")                                                 \
X(S_FP_ERR_TIMEOUT,                                                                       \
  L"探测超时（120 秒）。网络太慢，或者 yt-dlp 卡住了。",                                    \
  L"Timed out after 120 s. Your connection is slow, or yt-dlp is stuck.",                 \
  L"探測逾時（120 秒）。網路太慢，或者 yt-dlp 卡住了。",                                    \
  L"120 秒でタイムアウトしました。回線が遅いか、yt-dlp が停止しています。",                  \
  L"120초 후 시간이 초과되었습니다. 연결이 느리거나 yt-dlp가 멈췄습니다.")                    \
X(S_FP_ERR_NOOUTPUT,                                                                      \
  L"yt-dlp 没有任何输出。", L"yt-dlp produced no output.", L"yt-dlp 沒有任何輸出。",         \
  L"yt-dlp が何も出力しませんでした。", L"yt-dlp가 아무 출력도 내지 않았습니다.")             \
X(S_FP_ERR_PARSE,                                                                         \
  L"解析失败。", L"Could not parse the response.", L"解析失敗。",                           \
  L"応答を解析できませんでした。", L"응답을 해석하지 못했습니다.")                            \
                                                                                          \
/* ---------------------------------------------------------------- 默认字幕语言 */       \
/* 各语言的默认订阅语种不同：日文用户默认要日文字幕，以此类推。 */                          \
X(S_DEFAULT_SUBLANGS,                                                                     \
  L"zh-Hans,zh-Hant,zh-CN,zh,en", L"en", L"zh-Hant,zh-TW,zh,en",                          \
  L"ja,en", L"ko,en")
