{
  description = "TPCAS parser toolkit (links against hydrastro/ds)";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixpkgs-unstable";
    ds.url = "github:hydrastro/ds";
  };

  outputs = { self, nixpkgs, ds }:
    let
      systems = [ "x86_64-linux" "aarch64-linux" "x86_64-darwin" "aarch64-darwin" ];
      forAllSystems = nixpkgs.lib.genAttrs systems;

      # Resolve the ds package regardless of which attribute upstream exposes.
      dsFor = system:
        if ds.packages.${system} ? default then ds.packages.${system}.default
        else if ds.packages.${system} ? ds then ds.packages.${system}.ds
        else ds.defaultPackage.${system};

      # ds installs headers under $out/include (with the lib/ prefix the
      # sources include, e.g. "lib/str.h") and its archive/shared object
      # under $out/lib. Expose both to make(1) via overridable variables.
      dsFlags = dsPackage: {
        DS_CFLAGS = "-I${dsPackage}/include";
        DS_LIBS = "-L${dsPackage}/lib -lds";
      };
    in {
      packages = forAllSystems (system:
        let
          pkgs = import nixpkgs { inherit system; };
          dsPackage = dsFor system;
          f = dsFlags dsPackage;
        in {
          default = pkgs.stdenv.mkDerivation {
            pname = "tpcas";
            version = "0.1.0";
            src = ./.;

            nativeBuildInputs = [ pkgs.gnumake ];
            buildInputs = [ dsPackage ];

            DS_CFLAGS = f.DS_CFLAGS;
            DS_LIBS = f.DS_LIBS;

            buildPhase = ''
              make MODE=release
            '';

            doCheck = true;
            checkPhase = ''
              make check
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
          dsPackage = dsFor system;
          f = dsFlags dsPackage;
        in {
          default = pkgs.mkShell {
            buildInputs = [ dsPackage ];
            nativeBuildInputs = [
              pkgs.gnumake
              pkgs.clang-tools
              pkgs.gdb
              pkgs.valgrind
            ];

            DS_CFLAGS = f.DS_CFLAGS;
            DS_LIBS = f.DS_LIBS;
          };
        });
    };
}
