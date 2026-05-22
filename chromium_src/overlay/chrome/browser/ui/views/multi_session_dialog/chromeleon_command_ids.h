// chromium_src/overlay/chrome/browser/ui/views/multi_session_dialog/chromeleon_command_ids.h
// Chromeleon 専用コマンド ID 定義。
//
// Chromium 本体の IDC_* 値は chrome/app/chrome_command_ids.h で管理される。
// M135 時点の最大値は IDC_FIRST_UNBOUNDED_MENU (55000) 台であるため、
// Chromeleon 用には 57100 番台を予約する。
// 将来の Chromium 更新でコンフリクトが生じた場合はこのヘッダを修正する。
//
// Usage:
//   #include "chromium_src/overlay/chrome/browser/ui/views/multi_session_dialog/chromeleon_command_ids.h"

#ifndef CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_UI_VIEWS_MULTI_SESSION_DIALOG_CHROMELEON_COMMAND_IDS_H_
#define CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_UI_VIEWS_MULTI_SESSION_DIALOG_CHROMELEON_COMMAND_IDS_H_

// コンテキストメニュー: 「このリンクを複数セッションで開く」
// 範囲: 57100–57199 を Chromeleon 用として予約。
constexpr int IDC_CONTENT_CONTEXT_OPENLINK_MULTI_SESSION = 57100;

#endif  // CHROMIUM_SRC_OVERLAY_CHROME_BROWSER_UI_VIEWS_MULTI_SESSION_DIALOG_CHROMELEON_COMMAND_IDS_H_
