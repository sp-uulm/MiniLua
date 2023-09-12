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
pkgs.stdenv.mkDerivation {
  pname = "MiniLua";
  version = "0.1.0";
  src = with pkgs.lib; cleanSourceWith {
    src = cleanSource ./.;
    filter = path: type: path != toString ./. + "/build";
  };

  nativeBuildInputs = with pkgs; [
    cmake

    # Dependencies
    boost
    qt6.qtbase
    qt6.qmake
    qt6.wrapQtAppsHook
  ];
}
