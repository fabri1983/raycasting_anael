@echo off
setlocal enabledelayedexpansion

:: Input and output file paths
set input_image=doom_title_screen_full.png
set output_image=%input_image:.png=%_shifted.png

:: Temporary directories
set columns_dir=temp_columns
rmdir /s /q %columns_dir% >nul 2>&1
mkdir %columns_dir% >nul 2>&1

set split_shift_dir=temp_split_shift_dir
rmdir /s /q %split_shift_dir% >nul 2>&1
mkdir %split_shift_dir% >nul 2>&1

set strips_dir=%split_shift_dir%\temp_strips
rmdir /s /q %strips_dir% >nul 2>&1
mkdir %strips_dir% >nul 2>&1

:: Pixels wide for each column
set column_width=8

:: Offsets for each column
set offsets=8 0 24 32 16 8 0 4 24 32 8 16 24 8 24 16 32 8 16 8 32 0 24 16 32 16 0 8 16 24 8 24 0 32 16 8 32 8 24 8

:: Calculate the number of offsets
set /a offset_count=0
for %%i in (%offsets%) do (
    set /a offset_count+=1
)
set /a offset_count_minus_1=%offset_count%-1

:: Get image dimensions
for /f "tokens=1,2 delims=x" %%a in ('magick identify -format "%%wx%%h" %input_image%') do (
    set image_width=%%a
    set image_height=%%b
)

echo %input_image% %image_width%x%image_height%

:: Verify if offsets fit within the image width
set /a total_width=%offset_count% * %column_width%
if %total_width% GTR %image_width% (
    echo Error: Offset count exceeds the image width: %total_width% ^> %image_width%
	:: Cleanup temporary files
	rmdir /s /q %columns_dir%
    exit /b 1
)

:: Process each column
set /a column_index=0
for %%i in (%offsets%) do (
    set /a column_x=!column_index! * %column_width%
    set abs_offset=%%i
    if !abs_offset! LSS 0 set abs_offset=!abs_offset:~1!

    :: Extract the column
    magick convert %input_image% -crop %column_width%x%image_height%+!column_x!+0 %columns_dir%\column_!column_index!.png

    :: Shift the column vertically downward
	magick convert %columns_dir%\column_!column_index!.png -background "rgba(0,0,0,0)" -gravity North ^
		-splice 0x!abs_offset! -crop %column_width%x%image_height%+!column_x!+0 +repage %columns_dir%\shifted_column_!column_index!.png
	
    set /a column_index+=1
)

:: Merge all shifted columns into one image
echo Merging columns into final image...
set merge_command=magick convert
for /l %%i in (0,1,%offset_count_minus_1%) do (
    set merge_command=!merge_command! %columns_dir%\shifted_column_%%i.png
)
set merge_command=!merge_command! +append %split_shift_dir%\%output_image%
call !merge_command!

:: Check if the output image was created
if not exist %split_shift_dir%\%output_image% (
    echo Error: Failed to create final output image.
	:: Cleanup temporary files
	rmdir /s /q %columns_dir%
    exit /b 1
)

:: Cleanup temporary files
rmdir /s /q %columns_dir%


:: ###############################################################


echo Now generating splits from %split_shift_dir%\%output_image% while continue doing the shifting all the way down ...

set /a MELTING_OFFSET_STEPPING=4

:: Extract image name by removing the .png extension from %output_image%
set target_img_name=%output_image:.png=%
echo Shifted image name prefix is: %target_img_name%

:: Calculate number of vertical shifts needed to move image fully off canvas
:: Each shift is MELTING_OFFSET_STEPPING pixels
set /a num_vert_shifts=(%image_height% + (%MELTING_OFFSET_STEPPING%-1)) / %MELTING_OFFSET_STEPPING%

:: Generate shifted images
echo Generating shifted images in %split_shift_dir% ...
for /l %%i in (0,1,%num_vert_shifts%) do (
	set /a offset=%%i * %MELTING_OFFSET_STEPPING%
	magick convert %split_shift_dir%\%output_image% -background "rgba(0,0,0,0)" -gravity North ^
		-splice 0x!offset! -crop %image_width%x%image_height%+0+0 +repage ^
		%split_shift_dir%\%target_img_name%_%%i.png
)

del /f /q %split_shift_dir%\%output_image%

echo Shifted images saved to %split_shift_dir%\%target_img_name%_*.png


:: ###############################################################


set rowsPerStrip=8
echo Now for each shifted image we gonna split them in strips of %image_width%x8 ...

:: for each file with name pattern %target_img_name%_*.png located at %split_shift_dir% folder run next command
for %%f in ("%split_shift_dir%\%target_img_name%_*.png") do (
    magick "%%f" -set filename:base "%%[basename]" -unique -depth 8 -colors 256 -crop %image_width%x%rowsPerStrip% -define png:format=png8 -define histogram:unique-colors=false "%strips_dir%\%%[filename:base]_%%d.png"
)

echo Strips images saved to %strips_dir%
