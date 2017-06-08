function(LoadVersion FILEPATH PREFIX)
    # E.g., "BSON_VERSION".
    string(REPLACE ";" "" VERSION_NAME ${PREFIX} _VERSION)

    file(STRINGS ${FILEPATH} VERSION_CONTENTS)

    # A list of version components separated by dots and dashes: "1.3.0-dev"
    string(REGEX MATCHALL "[^.-]+" VERSION ${VERSION_CONTENTS})

    list(GET VERSION 0 MAJOR_VERSION)
    string(REPLACE ";" "" MAJOR_VERSION_NAME ${PREFIX} _MAJOR_VERSION)
    set(${MAJOR_VERSION_NAME} ${MAJOR_VERSION} PARENT_SCOPE)

    list(GET VERSION 1 MINOR_VERSION)
    string(REPLACE ";" "" MINOR_VERSION_NAME ${PREFIX} _MINOR_VERSION)
    set(${MINOR_VERSION_NAME} ${MINOR_VERSION} PARENT_SCOPE)

    list(GET VERSION 2 MICRO_VERSION)
    string(REPLACE ";" "" MICRO_VERSION_NAME ${PREFIX} _MICRO_VERSION)
    set(${MICRO_VERSION_NAME} ${MICRO_VERSION} PARENT_SCOPE)

    string(REPLACE ";" "" PRERELEASE_VERSION_NAME ${PREFIX} _PRERELEASE_VERSION)
    list(LENGTH VERSION VERSION_LENGTH)
    if(VERSION_LENGTH GREATER 3)
        list(GET VERSION 3 PRERELEASE_VERSION)
        set(${PRERELEASE_VERSION_NAME} ${PRERELEASE_VERSION} PARENT_SCOPE)
        set(${VERSION_NAME}
            "${MAJOR_VERSION}.${MINOR_VERSION}.${MICRO_VERSION}-${PRERELEASE_VERSION}"
            PARENT_SCOPE)
    else()
        set(${PRERELEASE_VERSION_NAME} "" PARENT_SCOPE)
        set(${VERSION_NAME}
            "${MAJOR_VERSION}.${MINOR_VERSION}.${MICRO_VERSION}"
            PARENT_SCOPE)
    endif()
endfunction(LoadVersion)
