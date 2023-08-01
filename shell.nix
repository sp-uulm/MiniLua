{
  sources ? import ./nix/sources.nix,
  system ? builtins.currentSystem,
  pkgs ?
    import sources.nixpkgs {
      overlays = [];
      config = {};
      inherit system;
    },
}:
pkgs.mkShell {
  nativeBuildInputs = with pkgs; [
    # Build tools
    niv
    git
    cmake

    # Dependencies
    boost
    qt5.qtbase
    qt5.qmake

    # Tools
    clang-tools
    python3
    lcov
    gcovr
    doxygen
    which
    curl
  ];

  IN_MINILUA_NIX_SHELL = true;
}
