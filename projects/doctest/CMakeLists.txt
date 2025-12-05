include(FetchContent)

# dotest
FetchContent_Declare(
    doctest
    GIT_REPOSITORY "https://github.com/doctest/doctest.git"
    GIT_TAG "1da23a3e8119ec5cce4f9388e91b065e20bf06f5"
    EXCLUDE_FROM_ALL
)

FetchContent_MakeAvailable(doctest)
