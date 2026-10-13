修正版LaTeX一式（コンパイル検証済み）

ZIPを解凍し、ターミナルでこのフォルダへ移動して実行してください。

bash build.command

LuaLaTeX → upBibTeX → LuaLaTeX → LuaLaTeXを自動実行し、main.pdfを生成します。
途中のエラーや未解決参照があると停止します。main.texは改名せず使用してください。

手動実行：
lualatex -interaction=nonstopmode -halt-on-error main.tex
upbibtex main
lualatex -interaction=nonstopmode -halt-on-error main.tex
lualatex -interaction=nonstopmode -halt-on-error main.tex

修正・検証：
・図を序論直後へ移動し、floatパッケージの[H]で固定。
・文献3件と本文引用、図のlabel/refの対応を確認。
・BibTeX処理済みのmain.bblと確認済みPDFを同梱。
・geometry=userでクラスの自動丸めを無効にし、指定寸法を厳密に設定。
・先頭ページのページ番号も非表示に統一。
・段の高さをそろえるための不要な縦方向の引き伸ばしを抑制。
・箇条書き見出しと本文の間隔を調整。
・指定書式へ変更：上24mm、下25mm、左15mm、右15mm、段間8mm。
・本文領域は180mm×248mm、各段の幅は86mm。
・本文のみの段で約46行となる行送り（約15.46pt）に設定。
・タイトル、図、見出しが占める部分は本文の実行数から除かれます。
・ログの記録項目、騒音対策、評価条件を補足し、左右の段の下端近くまで本文を配置。
・今後の計画は3項目で構成し、A4・1ページに収録。
・指定余白、本文10pt、段間8mm、約46行の行送りは維持。
・最終ログにエラー、警告、未解決参照、文字欠落、Overfull/Underfullなし。
・PDF画像で序論直下の図、図1、文献[1]～[3]を確認。

必要環境：LuaLaTeX、upBibTeX、日本語TeXパッケージ、TikZ、float、enumitem。
TeX Live 2023系で検証済み。環境によって改行位置が変わる場合があります。
元原稿の2.5～4.0秒は参考値です。測定記録の確認は別途必要です。
