#include "ConfigLoader.h"
#include "test_helpers.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <string>

// Helper to create a temporary config file
static std::string createTempConfig(const std::string& content, const std::string& filename = "test_config.cfg") {
    std::filesystem::create_directories("config/tests");
    std::string actualPath = "config/tests/temp_" + filename;
    std::ofstream file(actualPath);
    file << content;
    file.close();
    return "tests/temp_" + filename; // Return relative to config dir
}

static void cleanupTempFile(const std::string& path) {
    std::filesystem::remove("config/" + path);
}

// ============================================================================
// Basic config loading tests
// ============================================================================

void testBasicConfigLoading() {
    std::cout << "  [TEST] Basic config file loading... ";
    
    std::string content = "# Test config\nwidth=1920\nheight=1080\n";
    std::string path = createTempConfig(content);
    
    ConfigLoader cfg(path);
    assert(cfg.hasVar("width"));
    assert(cfg.hasVar("height"));
    
    cleanupTempFile(path);
    std::cout << "PASSED" << std::endl;
}

void testBoolParsing() {
    std::cout << "  [TEST] Boolean value parsing... ";
    
    std::string content = "fullscreen=true\nvsync=False\ndebug=1\nquiet=0\n";
    std::string path = createTempConfig(content);
    
    ConfigLoader cfg(path);
    assert(cfg.getBool("fullscreen") == true);
    assert(cfg.getBool("vsync") == false);
    assert(cfg.getBool("debug") == true);
    assert(cfg.getBool("quiet") == false);
    
    cleanupTempFile(path);
    std::cout << "PASSED" << std::endl;
}

void testIntParsing() {
    std::cout << "  [TEST] Integer value parsing... ";
    
    std::string content = "width=1920\nheight=-1080\ndepth=0\noffset=+500\n";
    std::string path = createTempConfig(content);
    
    ConfigLoader cfg(path);
    assert(cfg.getInt("width") == 1920);
    assert(cfg.getInt("height") == -1080);
    assert(cfg.getInt("depth") == 0);
    assert(cfg.getInt("offset") == 500);
    
    cleanupTempFile(path);
    std::cout << "PASSED" << std::endl;
}

void testFloatParsing() {
    std::cout << "  [TEST] Float value parsing... ";
    
    std::string content = "fov=45.5\ndistance=0.0\nscale=-1.5\n";
    std::string path = createTempConfig(content);
    
    ConfigLoader cfg(path);
    assert(approxEqual(cfg.getFloat("fov"), 45.5f));
    assert(approxEqual(cfg.getFloat("distance"), 0.0f));
    assert(approxEqual(cfg.getFloat("scale"), -1.5f));
    
    cleanupTempFile(path);
    std::cout << "PASSED" << std::endl;
}

void testStringParsing() {
    std::cout << "  [TEST] String value parsing... ";
    
    std::string content = "name=My Application\nurl=https://example.com\n";
    std::string path = createTempConfig(content);
    
    ConfigLoader cfg(path);
    std::string name = std::string(cfg.getVar("name"));
    assert(name == "My Application");
    
    cleanupTempFile(path);
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Whitespace handling tests
// ============================================================================

void testWhitespaceTrimming() {
    std::cout << "  [TEST] Whitespace trimming... ";
    
    std::string content = "  key1  =  value1  \n\tkey2\t=\tvalue2\t\n";
    std::string path = createTempConfig(content);
    
    ConfigLoader cfg(path);
    assert(cfg.hasVar("key1"));
    assert(cfg.hasVar("key2"));
    
    std::string val1 = std::string(cfg.getVar("key1"));
    assert(val1 == "value1");
    
    cleanupTempFile(path);
    std::cout << "PASSED" << std::endl;
}

void testEmptyLinesAndComments() {
    std::cout << "  [TEST] Empty lines and comments handling... ";
    
    std::string content = "\n# Comment line\n\n   # Indented comment\nkey=value\n\n";
    std::string path = createTempConfig(content);
    
    ConfigLoader cfg(path);
    assert(cfg.hasVar("key"));
    assert(std::string(cfg.getVar("key")) == "value");
    
    cleanupTempFile(path);
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Edge case tests
// ============================================================================

void testMissingKey() {
    std::cout << "  [TEST] Missing key handling... ";
    
    std::string content = "key=value\n";
    std::string path = createTempConfig(content);
    
    ConfigLoader cfg(path);
    assert(cfg.hasVar("key") == true);
    assert(cfg.hasVar("nonexistent") == false);
    
    // getBool returns false for missing keys
    assert(cfg.getBool("nonexistent") == false);
    
    // getInt returns 0 for missing keys
    assert(cfg.getInt("nonexistent") == 0);
    
    // getFloat returns 0.0f for missing keys
    assert(cfg.getFloat("nonexistent") == 0.0f);
    
    cleanupTempFile(path);
    std::cout << "PASSED" << std::endl;
}

void testEmptyConfigFile() {
    std::cout << "  [TEST] Empty config file... ";
    
    std::string content = "";
    std::string path = createTempConfig(content);
    
    ConfigLoader cfg(path);
    assert(cfg.hasVar("anykey") == false);
    
    cleanupTempFile(path);
    std::cout << "PASSED" << std::endl;
}

void testCommentsOnly() {
    std::cout << "  [TEST] Comments-only config file... ";
    
    std::string content = "# Only comments\n# No data here\n";
    std::string path = createTempConfig(content);
    
    ConfigLoader cfg(path);
    assert(cfg.hasVar("anykey") == false);
    
    cleanupTempFile(path);
    std::cout << "PASSED" << std::endl;
}

void testMultipleEquals() {
    std::cout << "  [TEST] Multiple equals signs... ";
    
    std::string content = "url=https://example.com?foo=bar&baz=qux\n";
    std::string path = createTempConfig(content);
    
    ConfigLoader cfg(path);
    std::string val = std::string(cfg.getVar("url"));
    assert(val == "https://example.com?foo=bar&baz=qux");
    
    cleanupTempFile(path);
    std::cout << "PASSED" << std::endl;
}

void testSpecialCharacters() {
    std::cout << "  [TEST] Special characters in values... ";
    
    std::string content = "emoji=Hello World!\nsymbols=@#$%^&*\n";
    std::string path = createTempConfig(content);
    
    ConfigLoader cfg(path);
    assert(cfg.hasVar("emoji"));
    assert(cfg.hasVar("symbols"));
    
    cleanupTempFile(path);
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Filename tests
// ============================================================================

void testFilenameTracking() {
    std::cout << "  [TEST] Filename tracking... ";
    
    std::string content = "key=value\n";
    std::string path = createTempConfig(content, "filename_test.cfg");
    
    ConfigLoader cfg(path);
    std::string fname = std::string(cfg.getFilename());
    assert(fname.find("filename_test.cfg") != std::string::npos);
    
    cleanupTempFile(path);
    std::cout << "PASSED" << std::endl;
}

void testNonexistentFile() {
    std::cout << "  [TEST] Nonexistent file handling... ";
    
    ConfigLoader cfg("this_file_does_not_exist.cfg");
    assert(cfg.hasVar("anykey") == false);
    
    std::cout << "PASSED" << std::endl;
}

// ============================================================================
// Stream output test
// ============================================================================

void testStreamOutput() {
    std::cout << "  [TEST] Stream output operator... ";
    
    std::string content = "width=1920\nheight=1080\n";
    std::string path = createTempConfig(content);
    
    ConfigLoader cfg(path);
    
    // Capture stream output
    std::ostringstream oss;
    oss << cfg;
    std::string output = oss.str();
    
    assert(output.find("width") != std::string::npos);
    assert(output.find("1920") != std::string::npos);
    assert(output.find("height") != std::string::npos);
    assert(output.find("1080") != std::string::npos);
    
    cleanupTempFile(path);
    std::cout << "PASSED" << std::endl;
}



// ============================================================================
// Test runner
// ============================================================================

int runConfigLoaderTests() {
    std::cout << "\n=== ConfigLoader Tests ===" << std::endl;
    
    testBasicConfigLoading();
    testBoolParsing();
    testIntParsing();
    testFloatParsing();
    testStringParsing();
    testWhitespaceTrimming();
    testEmptyLinesAndComments();
    testMissingKey();
    testEmptyConfigFile();
    testCommentsOnly();
    testMultipleEquals();
    testSpecialCharacters();
    testFilenameTracking();
    testNonexistentFile();
    testStreamOutput();
    
    // Cleanup any remaining temp files
    cleanupTempFile("tests/temp_test_config.cfg");
    cleanupTempFile("tests/temp_filename_test.cfg");
    
    std::cout << "All ConfigLoader tests PASSED!" << std::endl;
    return 0;
}
