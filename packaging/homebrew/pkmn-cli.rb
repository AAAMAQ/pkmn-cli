# typed: strict
# frozen_string_literal: true

# Versioned source formula; prebuilt bottles are not published yet.
class PkmnCli < Formula
  desc "Translate Pokemon Red or Blue saves into FireRed or LeafGreen"
  homepage "https://github.com/AAAMAQ/pkmn-cli"
  url "https://github.com/AAAMAQ/pkmn-cli/archive/refs/tags/v3.1.0.tar.gz"
  sha256 "94c28140c1d49c9b08542deb43b7b5c19dc914e93c7e142a9d21074a9dbb6bfe"
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
