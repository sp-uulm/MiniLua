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
		niv
		clang-tools 
		python3
	];
}
