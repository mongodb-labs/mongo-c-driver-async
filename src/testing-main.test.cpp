#include <catch2/catch_session.hpp>
#include <catch2/internal/catch_clara.hpp>

#include <test_params.test.hpp>

int main(int argc, char** argv) {
    Catch::Session session;
    std::string    uri;
    auto           cli = session.cli()
        | Catch::Clara::Opt(amongoc::testing::parameters.mongodb_uri, "uri")["--mongodb-uri"](
                   "Specify a MongoDB server URI to use for testing ($AMONGOC_TEST_MONGODB_URI)");
    session.cli(cli);
    int rc = session.applyCommandLine(argc, argv);
    if (rc) {
        return rc;
    }
    int r = session.run(argc, argv);
    return r;
}
