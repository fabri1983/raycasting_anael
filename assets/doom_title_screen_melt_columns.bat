@echo off
setlocal enabledelayedexpansion

:: Input and output file paths
set input_image=doom_title_screen_full.png
set output_image=doom_title_screen_full_shifted.png

:: Temporary working directory
set work_dir=temp_columns
if not exist %work_dir% mkdir %work_dir%

:: Pixels wide for each column
set column_width=16

:: Offsets for each column
set offsets=4 20 12 0 20 4 12 16 20 14 8 24 20 4 16 12 0 16 4 12

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

echo %image_width%x%image_height%

:: Verify if offsets fit within the image width
set /a total_width=%offset_count% * %column_width%
if %total_width% GTR %image_width% (
    echo Error: Offset count exceeds the image width: %total_width% ^> %image_width%
	:: Cleanup temporary files
	rmdir /s /q %work_dir%
    exit /b 1
)

:: Process each column
set /a column_index=0
for %%i in (%offsets%) do (
    set /a column_x=!column_index! * %column_width%
    set abs_offset=%%i
    if !abs_offset! LSS 0 set abs_offset=!abs_offset:~1!

    :: Extract the column
    magick convert %input_image% -crop %column_width%x%image_height%+!column_x!+0 %work_dir%\column_!column_index!.png

    :: Shift the column vertically downward
	magick convert %work_dir%\column_!column_index!.png -background "rgba(0,0,0,0)" -gravity North ^
		-splice 0x!abs_offset! -crop %column_width%x%image_height%+!column_x!+0 +repage %work_dir%\shifted_column_!column_index!.png
	
    set /a column_index+=1
)

:: Merge all shifted columns into one image
echo Merging columns into final image...
set merge_command=magick convert
for /l %%i in (0,1,%offset_count_minus_1%) do (
    set merge_command=!merge_command! %work_dir%\shifted_column_%%i.png
)
set merge_command=!merge_command! +append %output_image%
call !merge_command!

:: Check if the output image was created
if not exist %output_image% (
    echo Error: Failed to create final output image.
	:: Cleanup temporary files
	rmdir /s /q %work_dir%
    exit /b 1
)

:: Cleanup temporary files
rmdir /s /q %work_dir%

echo Output saved to %output_image%
