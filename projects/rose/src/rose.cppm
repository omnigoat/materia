export module rose;

export import :path;
export import :file;
export import :console;
export import :runtime;
export import :mmap;

#if defined(ATMA_PLATFORM_WINDOWS) && ATMA_PLATFORM_WINDOWS
export import :mmap_windows;
export import :console_windows;
export import :file_windows;
#endif
