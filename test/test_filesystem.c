void test_filesystem_create_directory() {
    const char* directory = "./foo/bar/quux";
    fs_create_directory_recursive(directory);
}