{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
    
    nativeBuildInputs = with pkgs; [ 
      libevdev
      cmake pkg-config 
      #gcc10
      clang_11
    ];

    shellHook = ''
        CC=${pkgs.clang_11}/bin/cc
        CXX=${pkgs.clang_11}/bin/c++
    '';
}