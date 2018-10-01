From https://github.com/yhirose/cpp-httplib/commit/d32eee76274d9c8f7e3c6b690b3de53ff8445e81

MIT License

===

cpp-httplib

A C++11 header-only HTTP library.

It's extremely easy to setup. Just include httplib.h file in your code!

Inspired by Sinatra and express.

© 2017 Yuji Hirose

===

Additional changes to fit our needs:

Add the functions to Client:
- void set_verify(SSLVerifyMode mode)
- void add_client_cert_ASN1(std::vector<unsigned char> cert, std::vector<unsigned char> key)
- void add_cert(std::vector<unsigned char> cert)

Add SSLVerifyMode enum
