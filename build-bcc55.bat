rem bcc32 -W -DUNICODE -D_UNICODE -O1 Typaste.cpp AutoType.cpp shlwapi.lib
bcc32 -W -D_MBCS -O1 Typaste.cpp AutoType.cpp shlwapi.lib
brc32 Typaste_res.rc Typaste.exe
