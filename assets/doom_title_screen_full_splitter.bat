@echo off
setlocal enabledelayedexpansion

:: Input and image name
set input_image=doom_title_screen_full.png
set image_name=%input_image:.png=%

:: Output dir
set output_dir=temp_strips
mkdir %output_dir% >nul 2>&1

magick %input_image% -unique -depth 8 -colors 256 -crop 320x8 -define png:format=png8 -define histogram:unique-colors=false %output_dir%/%image_name%_0_%%d.png
