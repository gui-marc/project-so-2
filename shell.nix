{ pkgs ? import <nixpkgs> { } }:
pkgs.mkShell {
  buildInputs = with pkgs;
    [
      cmake
      gdb
      gcc9
      clang-tools
      valgrind
    ];
}
