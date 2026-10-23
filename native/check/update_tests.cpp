// cos_update_tests: host tests of the update's pure part (native/src/pc/features/pc_update_core.h,
// issues #27 and #28, docs/AUTO_UPDATE_PLAN.md):
//
//   ninja -C build/native-mac cos_update_tests && build/native-mac/cos_update_tests
//
// Versions and their order, SHA-256 (FIPS 180-4's examples), the GitHub release JSON, the install
// names, the zip reader (stored and deflated entries, CRC, truncated and non-zip files) and the
// install journal. pc_update_core.cpp needs no game code, so this is a plain host program (cos_pc_tests
// links the game's operator new, which needs JKRHeap). Prints "ok" and exits 0 when every check
// passes; otherwise prints each failed check and exits 1.
#include "../src/pc/features/pc_update_core.h"

#include <zlib.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>
#include <utility>
#include <vector>

namespace {

int g_failures = 0;

#define CHECK(expr)                                                                                \
    do {                                                                                           \
        if (!(expr)) {                                                                             \
            std::fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, #expr);         \
            ++g_failures;                                                                          \
        }                                                                                          \
    } while (0)


// A zip as `zip -qr` writes one: local headers, data, central directory, end record. deflated=true
// stores each entry deflated (raw, as zip does).
std::string makeZip(const std::vector<std::pair<std::string, std::string>>& files, bool deflated) {
    auto put16 = [](std::string& s, uint32_t v) { s += (char)(v & 0xFF); s += (char)((v >> 8) & 0xFF); };
    auto put32 = [&](std::string& s, uint32_t v) { put16(s, v & 0xFFFF); put16(s, v >> 16); };
    std::string out, cd;
    for (const auto& [name, data] : files) {
        std::string packed = data;
        if (deflated) {
            z_stream zs{};
            deflateInit2(&zs, Z_BEST_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, Z_DEFAULT_STRATEGY);
            packed.resize(deflateBound(&zs, (uLong)data.size()));
            zs.next_in = (Bytef*)data.data();
            zs.avail_in = (uInt)data.size();
            zs.next_out = (Bytef*)packed.data();
            zs.avail_out = (uInt)packed.size();
            deflate(&zs, Z_FINISH);
            packed.resize(zs.total_out);
            deflateEnd(&zs);
        }
        const uint32_t crc = (uint32_t)crc32(0, (const Bytef*)data.data(), (uInt)data.size());
        const uint32_t offset = (uint32_t)out.size();
        put32(out, 0x04034b50); put16(out, 20); put16(out, 0); put16(out, deflated ? 8 : 0);
        put32(out, 0); put32(out, crc); put32(out, (uint32_t)packed.size()); put32(out, (uint32_t)data.size());
        put16(out, (uint32_t)name.size()); put16(out, 0);
        out += name;
        out += packed;
        put32(cd, 0x02014b50); put16(cd, 20); put16(cd, 20); put16(cd, 0); put16(cd, deflated ? 8 : 0);
        put32(cd, 0); put32(cd, crc); put32(cd, (uint32_t)packed.size()); put32(cd, (uint32_t)data.size());
        put16(cd, (uint32_t)name.size()); put16(cd, 0); put16(cd, 0); put16(cd, 0); put16(cd, 0);
        put32(cd, 0); put32(cd, offset);
        cd += name;
    }
    const uint32_t cdOffset = (uint32_t)out.size();
    out += cd;
    put32(out, 0x06054b50); put16(out, 0); put16(out, 0);
    put16(out, (uint32_t)files.size()); put16(out, (uint32_t)files.size());
    put32(out, (uint32_t)cd.size()); put32(out, cdOffset); put16(out, 0);
    return out;
}

std::string readAll(const char* path) {
    std::string s;
    if (FILE* f = std::fopen(path, "rb")) {
        char buf[4096];
        size_t n;
        while ((n = std::fread(buf, 1, sizeof(buf), f)) > 0) {
            s.append(buf, n);
        }
        std::fclose(f);
    }
    return s;
}

void testUpdateCore() {
    using namespace pc::update;
    // Versions: a build past a tag is not older than that release; only X.Y.Z builds check.
    {
        Version v, r;
        CHECK(parseVersion("0.5.0", &v) && v.major == 0 && v.minor == 5 && v.patch == 0 && v.after == 0);
        CHECK(parseVersion("v1.12.3", &v) && v.major == 1 && v.minor == 12 && v.patch == 3);
        CHECK(parseVersion("0.4.2-12", &v) && v.patch == 2 && v.after == 12);
        CHECK(parseVersion("0.5.0-3-gabcdef1", &v) && v.after == 3);
        CHECK(parseVersion("0.5.0-dirty", &v) && v.after == 0);
        CHECK(!parseVersion("dev", &v));
        CHECK(!parseVersion("ceadab8", &v));
        CHECK(!parseVersion("0.5", &v));
        CHECK(!parseVersion("0.5.0x", &v));
        CHECK(!parseVersion(nullptr, &v));
        parseVersion("0.5.0", &v);
        CHECK(parseVersion("v0.5.1", &r) && isNewer(r, v));
        CHECK(parseVersion("v0.6.0", &r) && isNewer(r, v));
        CHECK(parseVersion("v1.0.0", &r) && isNewer(r, v));
        CHECK(parseVersion("v0.5.0", &r) && !isNewer(r, v));
        CHECK(parseVersion("v0.4.9", &r) && !isNewer(r, v));
        parseVersion("0.5.0-7", &v);
        CHECK(parseVersion("v0.5.0", &r) && !isNewer(r, v));
        CHECK(parseVersion("v0.5.1", &r) && isNewer(r, v));
        CHECK(sameTag("v0.5.1", "0.5.1") && sameTag("0.5.1", "v0.5.1") && !sameTag("v0.5.1", "0.5.10"));
        CHECK(!sameTag("", ""));
    }
    // SHA-256: FIPS 180-4's examples, and a message across several blocks.
    {
        auto hex = [](const std::string& s) {
            Sha256 sha;
            sha.update(s.data(), s.size());
            uint8_t d[32];
            sha.final(d);
            return toHex(d, 32);
        };
        CHECK(hex("") == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
        CHECK(hex("abc") == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
        CHECK(hex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq") ==
              "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
        CHECK(hex(std::string(1000000, 'a')) == "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
        CHECK(parseSha256Line("51b43ba95c6a64ad0ff859e539f0aca1ffb4f3464cab18d83854ebc48991329b  SwitchWaker-v0.5.0-switch.zip\n") ==
              "51b43ba95c6a64ad0ff859e539f0aca1ffb4f3464cab18d83854ebc48991329b");
        CHECK(parseSha256Line("51B43BA95C6A64AD0FF859E539F0ACA1FFB4F3464CAB18D83854EBC48991329B") ==
              "51b43ba95c6a64ad0ff859e539f0aca1ffb4f3464cab18d83854ebc48991329b");
        CHECK(parseSha256Line("51b43ba9  x").empty());
        CHECK(parseSha256Line("<html>Not Found</html>").empty());
    }
    // The release JSON: the fields read, the rest skipped, escapes decoded; malformed answers fail.
    {
        const std::string json =
            "{\"url\":\"https://api.github.com/x\",\"html_url\":\"https://github.com/centollOS/SwitchWaker/releases/tag/v0.5.1\","
            "\"id\":123,\"author\":{\"login\":\"Pulpparty\",\"site_admin\":false,\"n\":null},\"tag_name\":\"v0.5.1\","
            "\"draft\":false,\"prerelease\":false,\"name\":\"SwitchWaker v0.5.1 \\u00e9\\ud83d\\ude00\\\"q\\\"\","
            "\"assets\":[{\"name\":\"SwitchWaker-v0.5.1-switch.zip\",\"size\":9518038,\"uploader\":{\"id\":1},"
            "\"browser_download_url\":\"https://github.com/centollOS/SwitchWaker/releases/download/v0.5.1/SwitchWaker-v0.5.1-switch.zip\"},"
            "{\"name\":\"SwitchWaker-v0.5.1-switch.zip.sha256\",\"size\":96,\"browser_download_url\":\"https://x/y.sha256\"}],"
            "\"body\":\"## Changes\\r\\n- a\\tb [x](y) \\/ \\\\\",\"reactions\":{\"+1\":2.5e1,\"list\":[1,-2,[true]]}}";
        Release r;
        std::string err;
        CHECK(parseRelease(json, &r, &err));
        CHECK(r.tag == "v0.5.1");
        CHECK(r.name == "SwitchWaker v0.5.1 \xC3\xA9\xF0\x9F\x98\x80\"q\"");
        CHECK(r.htmlUrl == "https://github.com/centollOS/SwitchWaker/releases/tag/v0.5.1");
        CHECK(r.assets.size() == 2);
        const Asset* a = findAsset(r, zipNameFor(r.tag, "GZLE01"));
        CHECK(a != nullptr && a->size == 9518038 &&
              a->url == "https://github.com/centollOS/SwitchWaker/releases/download/v0.5.1/SwitchWaker-v0.5.1-switch.zip");
        CHECK(findAsset(r, zipNameFor(r.tag, "GZLP01")) == nullptr);
        CHECK(zipNameFor("v0.5.1", "GZLP01") == "SwitchWaker-v0.5.1-switch-eur.zip");
        CHECK(zipNameFor("v0.5.1", "GZLJ01").empty());
        CHECK(parseRelease("{\"tag_name\":\"v1.0.0\",\"name\":null}", &r, &err) && r.name.empty() && r.assets.empty());
        CHECK(!parseRelease("{\"message\":\"API rate limit exceeded\"}", &r, &err) && err == "no tag_name");
        CHECK(!parseRelease("{\"tag_name\":\"v1\"", &r, &err));
        CHECK(!parseRelease("{\"tag_name\":\"v1\"} x", &r, &err));
        CHECK(!parseRelease("[1,2]", &r, &err));
        CHECK(!parseRelease("", &r, &err));
        CHECK(!parseRelease("<html>", &r, &err));
    }
    // Install names: only plain files under switch/switchwaker/.
    {
        std::string n;
        CHECK(installName("switch/switchwaker/switchwaker.nro", &n) && n == "switchwaker.nro");
        CHECK(installName("switch/switchwaker/initial_dksh_cache.bin", &n) && n == "initial_dksh_cache.bin");
        CHECK(!installName("switch/switchwaker/", &n));
        CHECK(!installName("switch/", &n));
        CHECK(!installName("switch/switchwaker/native/user/settings.ini", &n));
        CHECK(!installName("switch/switchwaker/..", &n));
        CHECK(!installName("switch/switchwaker/a\\b", &n));
        CHECK(!installName("switch/other/x.nro", &n));
        CHECK(!installName("../switch/switchwaker/x", &n));
    }
    // The zip reader: both methods, entries checked against their CRC.
    {
        std::string big;
        for (int i = 0; i < 300000; i++) {
            big += (char)('a' + (i * 7 + i / 13) % 26);
        }
        const std::vector<std::pair<std::string, std::string>> files = {
            {"switch/", ""}, {"switch/switchwaker/switchwaker.nro", big}, {"switch/switchwaker/empty.bin", ""},
            {"switch/switchwaker/initial_pipeline_cache.db", "pipeline"}};
        char dir[] = "/tmp/cos_update_test_XXXXXX";
        CHECK(mkdtemp(dir) != nullptr);
        const std::string zipPath = std::string(dir) + "/r.zip", outPath = std::string(dir) + "/out";
        for (bool deflated : {false, true}) {
            const std::string zip = makeZip(files, deflated);
            FILE* f = std::fopen(zipPath.c_str(), "wb");
            std::fwrite(zip.data(), 1, zip.size(), f);
            std::fclose(f);
            f = std::fopen(zipPath.c_str(), "rb");
            std::vector<ZipEntry> entries;
            std::string err;
            CHECK(zipList(f, &entries, &err) && entries.size() == 4);
            for (size_t k = 0; k < entries.size() && k < files.size(); k++) {
                CHECK(entries[k].name == files[k].first && entries[k].size == files[k].second.size());
                CHECK(zipExtract(f, entries[k], outPath.c_str(), &err));
                CHECK(readAll(outPath.c_str()) == files[k].second);
            }
            // A wrong CRC: the entry fails and leaves no file behind.
            ZipEntry bad = entries[1];
            bad.crc ^= 1;
            CHECK(!zipExtract(f, bad, outPath.c_str(), &err) && err.find("CRC") != std::string::npos);
            CHECK(access(outPath.c_str(), F_OK) != 0);
            std::fclose(f);
        }
        // Not a zip; a truncated zip.
        FILE* f = std::fopen(zipPath.c_str(), "wb");
        std::fputs("<html>Not Found</html>", f);
        std::fclose(f);
        f = std::fopen(zipPath.c_str(), "rb");
        std::vector<ZipEntry> entries;
        std::string err;
        CHECK(!zipList(f, &entries, &err));
        std::fclose(f);
        const std::string zip = makeZip(files, true);
        f = std::fopen(zipPath.c_str(), "wb");
        std::fwrite(zip.data(), 1, zip.size() / 2, f);
        std::fclose(f);
        f = std::fopen(zipPath.c_str(), "rb");
        CHECK(!zipList(f, &entries, &err));
        std::fclose(f);
        // sha256File agrees with the in-memory digest.
        f = std::fopen(zipPath.c_str(), "wb");
        std::fputs("abc", f);
        std::fclose(f);
        CHECK(sha256File(zipPath.c_str()) == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
        CHECK(sha256File((std::string(dir) + "/missing").c_str()).empty());
        std::remove(zipPath.c_str());
        std::remove(outPath.c_str());
        rmdir(dir);
    }
    // The journal round trip; bad journals are refused.
    {
        Journal j;
        j.state = "installing";
        j.tag = "v0.5.1";
        j.from = "0.5.0";
        j.names = {"switchwaker.nro", "initial_pipeline_cache.db", "initial_dksh_cache.bin"};
        j.existed = {"switchwaker.nro", "initial_pipeline_cache.db"};
        Journal back;
        CHECK(parseJournal(formatJournal(j), &back));
        CHECK(back.state == j.state && back.tag == j.tag && back.from == j.from && back.names == j.names &&
              back.existed == j.existed);
        CHECK(parseJournal("state=booting\r\ntag=v1.0.0\r\nnames=a.nro\r\n", &back) && back.existed.empty());
        CHECK(!parseJournal("state=weird\ntag=v1\nnames=a\n", &back));
        CHECK(!parseJournal("state=installed\nnames=a\n", &back));
        CHECK(!parseJournal("state=installed\ntag=v1\nnames=\n", &back));
        CHECK(!parseJournal("state=installed\ntag=v1\nnames=../x\n", &back));
        CHECK(!parseJournal("", &back));
    }
}

} // namespace

int main() {
    testUpdateCore();
    if (g_failures != 0) {
        std::printf("FAIL: %d checks\n", g_failures);
        return 1;
    }
    std::printf("ok\n");
    return 0;
}
