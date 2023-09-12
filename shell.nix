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
    qt6.qtbase
    qt6.qmake

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
