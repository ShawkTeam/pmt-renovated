/*
   Copyright 2025 Yağız Zengin

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#define PROGRAM_NAME "helper_test"

#include <iostream>
#include <libhelper/lib.hpp>

char *TEST_DIR = nullptr;

std::string test_path(const char *file) {
  std::string end = std::string(TEST_DIR) + "/" + file;
  return end;
}

int main(int argc, char **argv) {
  if (argc < 2) return 2;
  TEST_DIR = argv[1];

  try {
    std::cout << "Has super user?; " << std::boolalpha << Helper::hasSuperUser()
              << std::endl;
    std::cout << "file.txt is exists?; " << std::boolalpha
              << Helper::isExists(test_path("file.txt")) << std::endl;
    std::cout << "'file.txt' file is exists?; " << std::boolalpha
              << Helper::fileIsExists(test_path("file")) << std::endl;
    std::cout << "'dir' directory is exists?; " << std::boolalpha
              << Helper::directoryIsExists(test_path("dir")) << std::endl;
    std::cout << "'linkdir' is link?; " << std::boolalpha
              << Helper::isLink(test_path("linkdir")) << std::endl;
    std::cout << "'linkdir' is symlink?; " << std::boolalpha
              << Helper::isSymbolicLink(test_path("linkdir")) << std::endl;
    std::cout << "'linkdir' is hardlink?; " << std::boolalpha
              << Helper::isHardLink(test_path("linkdir")) << std::endl;
    std::cout << "'linkdir' is symlink to 'dir'?; " << std::boolalpha
              << Helper::areLinked(test_path("linkdir"), test_path("dir"))
              << std::endl;

    if (!Helper::writeFile("file.txt", "hello world"))
      throw Helper::Error("Cannot write \"hello world\" in 'file.txt'");
    else std::cout << "file.txt writed." << std::endl;

    if (const auto content = Helper::readFile("file.txt"); !content)
      throw Helper::Error("Cannot read 'file.txt'");
    else std::cout << "'file.txt': " << *content << std::endl;

    std::cout << "Making directory 'dir2': " << std::boolalpha
              << Helper::makeDirectory(test_path("dir2")) << std::endl;
    std::cout << "Making recursive directories 'dir3/x/y': " << std::boolalpha
              << Helper::makeRecursiveDirectory(test_path("dir3/x/y"))
              << std::endl;
    std::cout << "Create 'file2.txt': " << std::boolalpha
              << Helper::createFile(test_path("file2.txt")) << std::endl;
    std::cout << "Create symlink 'file2.txt' to 'file2lnk.txt': "
              << std::boolalpha
              << Helper::createSymlink(test_path("file2.txt"),
                                       test_path("file2lnk.txt"))
              << std::endl;
    std::cout << "Size of 'file2.txt': "
              << Helper::fileSize(test_path("file2.txt")) << std::endl;
    std::cout << "Erasing 'file.txt': " << std::boolalpha
              << Helper::eraseEntry(test_path("file.txt")) << std::endl;
    std::cout << "Erasing 'dir2': " << std::boolalpha
              << Helper::eraseEntry(test_path("dir2")) << std::endl;
    std::cout << "Read link of 'file2lnk.txt': "
              << Helper::readSymlink(test_path("file2lnk.txt")) << std::endl;

    if (const auto sha256 = Helper::sha256Of(test_path("file2.txt")); !sha256)
      throw Helper::Error("Cannot get sha256 of 'file2.txt'");
    else std::cout << "SHA256 of 'file2.txt': " << *sha256 << std::endl;

    std::cout << "'file2.txt' and 'file2lnk.txt' same? (SHA256): "
              << std::boolalpha
              << Helper::sha256Compare(test_path("file2.txt"),
                                       test_path("file2lnk.txt"))
              << std::endl;
    std::cout << "Copy 'file2.txt' as 'file2cpy.txt': " << std::boolalpha
              << Helper::copyFile(test_path("file2.txt"),
                                  test_path("file2cpy.txt"))
              << std::endl;
    std::cout << "Run command: 'ls': " << std::boolalpha
              << Helper::runCommand("ls") << std::endl;
    std::cout << "Spawn confirm propt..." << std::endl;

    const bool p = Helper::confirmPropt("Please answer");
    std::cout << "Result of confirm propt: " << std::boolalpha << p
              << std::endl;

    std::cout << "Working directory: " << Helper::currentWorkingDirectory()
              << std::endl;
    std::cout << "Current date: " << Helper::currentDate() << std::endl;
    std::cout << "Current time: " << Helper::currentTime() << std::endl;
    std::cout << "Output of 'ls' command: "
              << Helper::runCommandWithOutput("ls").first << std::endl;
    std::cout << "Basename of " << test_path("file2.txt") << ": "
              << Helper::pathBasename(test_path("file2.txt")) << std::endl;
    std::cout << "Dirname of " << test_path("file2.txt") << ": "
              << Helper::pathDirname(test_path("file2.txt")) << std::endl;

    std::cout << "pathJoin() test 1: " << Helper::pathJoin("mydir", "dir2")
              << std::endl;
    std::cout << "pathJoin() test 2: " << Helper::pathJoin("mydir/", "dir2")
              << std::endl;
    std::cout << "pathJoin() test 3: " << Helper::pathJoin("mydir/", "/dir2")
              << std::endl;
    std::cout << "pathJoin() test 4: " << Helper::pathJoin("mydir", "/dir2")
              << std::endl;

    Helper::PureTuple<int, std::string, bool> values = {
        {1, "hi", true}, {2, "im", true}, {3, "helper", false}};

    values.insert(std::make_tuple(0, "hi", false));
    values.insert(2, "im", true);
    values.insert({3, "helper", true});
    values.pop({3, "helper", true});
    values.pop_back();

    std::cout << "pure tuple test: " << std::boolalpha
              << static_cast<bool>(values.at(0)) << std::endl;
    for (const auto &[x, y, z] : values) {
      std::cout << std::boolalpha << "(" << x << ", " << y << ", " << z << ")"
                << std::endl;
    }

    std::cout << Helper::getLibVersion() << std::endl;

    LOG(INFO) << "Info message" << std::endl;
    LOG(WARNING) << "Warning message" << std::endl;
    LOG(ERROR) << "Error message" << std::endl;
    LOG(ABORT) << "Abort message" << std::endl;
  } catch (std::exception &err) {
    std::cout << err.what() << std::endl;
    return 1;
  }

  return 0;
}
