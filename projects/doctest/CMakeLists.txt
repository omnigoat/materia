include(FetchContent)

# dotest
FetchContent_Declare(
    doctest
    GIT_REPOSITORY "https://github.com/doctest/doctest.git"
    GIT_TAG "v2.5.1"
    EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(doctest)
