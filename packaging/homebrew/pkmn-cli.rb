# typed: strict
# frozen_string_literal: true

# Homebrew formula for the standalone Pokemon Red workflow CLI.
class PkmnCli < Formula
  desc "Standalone unified CLI for verified Pokemon Red save workflows"
  homepage "https://github.com/AAAMAQ/pkmn-cli"
  license "MIT"
  head "https://github.com/AAAMAQ/pkmn-cli.git", branch: "main"

  depends_on "cmake" => :build

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
  end
end
