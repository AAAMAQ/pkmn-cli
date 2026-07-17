class PkmnCli < Formula
  desc "Standalone unified CLI for verified Pokemon Red save workflows"
  homepage "https://github.com/REPLACE_WITH_RELEASE_REPOSITORY/pkmn-cli"
  url "https://github.com/REPLACE_WITH_RELEASE_REPOSITORY/pkmn-cli/archive/refs/tags/v0.1.0.tar.gz"
  sha256 "REPLACE_WITH_RELEASE_TARBALL_SHA256"
  license "MIT"

  depends_on "cmake" => :build

  def install
    system "cmake", "-S", ".", "-B", "build", *std_cmake_args
    system "cmake", "--build", "build", "--parallel"
    system "ctest", "--test-dir", "build", "--output-on-failure"
    system "cmake", "--install", "build"
    generate_completions_from_executable(bin/"pkmn", "completion")
  end

  test do
    assert_match "pkmn 0.1.0", shell_output("#{bin}/pkmn --version")
    assert_match "Standalone readiness: ready", shell_output("#{bin}/pkmn doctor")
  end
end
