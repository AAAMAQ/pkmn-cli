# typed: strict
# frozen_string_literal: true

# Homebrew formula for the Pokemon Red to FireRed workflow CLI.
class PkmnCli < Formula
  desc "Convert Pokemon Red saves to FireRed with auditable manifests"
  homepage "https://github.com/AAAMAQ/pkmn-cli"
  license "MIT"
  head "https://github.com/AAAMAQ/pkmn-cli.git", branch: "main"

  depends_on "cmake" => :build
  depends_on "python@3.13"

  def install
    system "cmake", "-S", ".", "-B", "build", *std_cmake_args
    system "cmake", "--build", "build", "--parallel"
    system "ctest", "--test-dir", "build", "--output-on-failure"
    system "cmake", "--install", "build"
    generate_completions_from_executable(bin/"pkmn", "completion")
  end

  test do
    assert_match "pkmn", shell_output("#{bin}/pkmn --version")
    assert_match "Standalone readiness: ready", shell_output("#{bin}/pkmn doctor")
    assert_match "0.4.0", shell_output("#{bin}/pkmn frjson schema")
  end
end
