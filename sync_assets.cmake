function(target_sync_assets target src dst)
    if (WIN32)
        add_custom_target(${target}
                COMMAND cmd /c
                "robocopy \"${src}\" \"${dst}\" /MIR /NFL /NDL /NJH /NJS /NC /NS & if %ERRORLEVEL% GEQ 8 exit %ERRORLEVEL%"
                COMMENT "Sync ${target} assets (robocopy)..."
        )
    else ()
        find_program(RSYNC rsync)
        if (RSYNC)
            add_custom_target(${target}
                    COMMAND ${RSYNC} -a --delete "${src}/" "${dst}/"
                    COMMENT "Sync ${target} assets (rsync)..."
            )
        else ()
            add_custom_target(${target}
                    COMMAND ${CMAKE_COMMAND} -E rm -rf "${dst}"
                    COMMAND ${CMAKE_COMMAND} -E copy_directory "${src}" "${dst}"
                    COMMENT "Sync ${target} assets (copy_directory)..."
            )
        endif ()
    endif ()
endfunction()