#include <iostream>
#include <string>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>

// Function to print OpenSSL errors
void printOpenSSLError() {
    ERR_print_errors_fp(stderr);
}

int main() {
    std::cout << "Generating RSA-4096 key pair, please wait (this may take a few seconds)..." << std::endl;

    // 1. Initialize EVP_PKEY context for RSA algorithm
    EVP_PKEY_CTX *ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, NULL);
    if (!ctx) {
        std::cerr << "Failed to create EVP_PKEY_CTX context!" << std::endl;
        printOpenSSLError();
        return 1;
    }

    // 2. Initialize key generation
    if (EVP_PKEY_keygen_init(ctx) <= 0) {
        std::cerr << "Failed to initialize key generation!" << std::endl;
        EVP_PKEY_CTX_free(ctx);
        printOpenSSLError();
        return 1;
    }

    // 3. Set RSA key length to 4096 bits
    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, 4096) <= 0) {
        std::cerr << "Failed to set key length!" << std::endl;
        EVP_PKEY_CTX_free(ctx);
        printOpenSSLError();
        return 1;
    }

    // 4. Generate the key pair
    EVP_PKEY *pkey = NULL;
    if (EVP_PKEY_keygen(ctx, &pkey) <= 0) {
        std::cerr << "Failed to generate key!" << std::endl;
        EVP_PKEY_CTX_free(ctx);
        printOpenSSLError();
        return 1;
    }

    // Free the context as it is no longer needed
    EVP_PKEY_CTX_free(ctx);

    // 5. Output private key to file (private_key.pem)
    BIO *bp_private = BIO_new_file("private_key.pem", "w+");
    if (!bp_private) {
        std::cerr << "Failed to create private_key.pem! Please check permissions." << std::endl;
        EVP_PKEY_free(pkey);
        return 1;
    }
    
    // Write private key in PEM format (no password encryption used here)
    if (!PEM_write_bio_PrivateKey(bp_private, pkey, NULL, NULL, 0, NULL, NULL)) {
        std::cerr << "Failed to write private key!" << std::endl;
        BIO_free_all(bp_private);
        EVP_PKEY_free(pkey);
        printOpenSSLError();
        return 1;
    }
    BIO_free_all(bp_private);

    // 6. Output public key to file (public_key.pem)
    BIO *bp_public = BIO_new_file("public_key.pem", "w+");
    if (!bp_public) {
        std::cerr << "Failed to create public_key.pem! Please check permissions." << std::endl;
        EVP_PKEY_free(pkey);
        return 1;
    }
    
    if (!PEM_write_bio_PUBKEY(bp_public, pkey)) {
        std::cerr << "Failed to write public key!" << std::endl;
        BIO_free_all(bp_public);
        EVP_PKEY_free(pkey);
        printOpenSSLError();
        return 1;
    }
    BIO_free_all(bp_public);

    // 7. Free the key structure
    EVP_PKEY_free(pkey);

    std::cout << "\nSuccess! RSA-4096 key pair has been generated in the current directory:" << std::endl;
    std::cout << " - private_key.pem (Private key, keep it safe and secret)" << std::endl;
    std::cout << " - public_key.pem (Public key, safe to share or deploy to server)" << std::endl;

    // Prevent the console window from closing immediately when double-clicked
    std::cout << "\nPress Enter to exit..." << std::endl;
    std::cin.get();

    return 0;
}