{
  description = "TPCAS parser toolkit with vendored DS allocator support";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
  };

  outputs = { self, nixpkgs }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
      forAllSystems = nixpkgs.lib.genAttrs systems;
    in {
      packages = forAllSystems (system:
        let
          pkgs = import nixpkgs { inherit system; };
        in {
          default = pkgs.stdenv.mkDerivation {
            pname = "tpcas";
            version = "0.1.0";
            src = ./.;

            nativeBuildInputs = [ pkgs.gnumake ];

            buildPhase = ''
              make MODE=release
            '';

            doCheck = true;
            checkPhase = ''
              make test
            '';

            installPhase = ''
              make install PREFIX=$out
            '';
          };
        });

      apps = forAllSystems (system: {
        default = {
          type = "app";
          program = "${self.packages.${system}.default}/bin/tpcas";
        };
      });

      devShells = forAllSystems (system:
        let
          pkgs = import nixpkgs { inherit system; };
        in {
          default = pkgs.mkShell {
            nativeBuildInputs = [
              pkgs.gnumake
              pkgs.clang-tools
              pkgs.gdb
              pkgs.valgrind
            ];
          };
        });
    };
}
