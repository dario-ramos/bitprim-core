/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_EC_KEYS_HPP
#define LIBBITCOIN_EC_KEYS_HPP

#include <cstddef>
#include <bitcoin/bitcoin/compat.hpp>
#include <bitcoin/bitcoin/define.hpp>
#include <bitcoin/bitcoin/math/hash.hpp>
#include <bitcoin/bitcoin/utility/data.hpp>

namespace libbitcoin {

/// Private keys:
BC_CONSTEXPR size_t ec_secret_size = 32;
typedef byte_array<ec_secret_size> ec_secret;

/// Public keys:
BC_CONSTEXPR size_t ec_compressed_size = 33;
typedef byte_array<ec_compressed_size> ec_compressed;

BC_CONSTEXPR size_t ec_uncompressed_size = 65;
typedef byte_array<ec_uncompressed_size> ec_uncompressed;

/// Full ecdsa signatures for input endorsement:
BC_CONSTEXPR size_t max_endorsement_size = 72;
typedef data_chunk endorsement;

/// Compact ecdsa signature for message signing:
BC_CONSTEXPR size_t compact_signature_size = 64;
typedef byte_array<compact_signature_size> compact_signature;

/// Trying out this compressed/decompressed slice.
class BC_API ec_public
{
public:
    ec_public(const ec_compressed& point)
      : point_(point.begin(), point.end())
    {
    }

    ec_public(const ec_uncompressed& point)
      : point_(point.begin(), point.end())
    {
    }

    const data_chunk& data() const
    {
        return point_;
    }

private:
    data_chunk point_;
};

// TODO: define string literal factory: ec_secret_literal().

/**
 * Verify whether data may be a public point.
 * Avoid allocations and point arithmetic.
 */
BC_API bool is_point(data_slice data);

/**
 * Decompresses a compressed public point.
 */
BC_API bool decompress(ec_uncompressed& out, const ec_compressed& point);

/**
 * Converts a secret to a compressed public point.
 */
BC_API bool secret_to_public(ec_compressed& out, const ec_secret& secret);

/**
 * Converts a secret parameter to an uncompressed public point.
 */
BC_API bool secret_to_public(ec_uncompressed& out, const ec_secret& secret);

/**
 * Verify a secret.
 */
BC_API bool verify(const ec_secret& secret);

/**
 * Verify a compressed or uncompressed point.
 */
BC_API bool verify(const ec_public& point);

/**
 * Create a deterministic signature using a private key.
 */
BC_API bool sign(endorsement& out, const ec_secret& secret,
    const hash_digest& hash);

/**
 * Verify a signature using a compressed or uncompressed point.
 */
BC_API bool verify_signature(const ec_public& point, const hash_digest& hash,
    const endorsement& signature);

/**
 * Create a compact signature using a private key.
 */
BC_API bool sign_compact(compact_signature& out_signature,
    uint8_t& out_recovery_id, const ec_secret& secret,
    const hash_digest& hash);

/**
 * Recovers the compressed point from a compact message signature.
 */
BC_API bool recover_public(ec_compressed& point,
    const compact_signature& signature, uint8_t recovery_id,
    const hash_digest& hash);

/**
 * Recovers the uncompressed point from a compact message signature.
 */
BC_API bool recover_public(ec_uncompressed& point,
    const compact_signature& signature, uint8_t out_recovery_id,
    const hash_digest& hash);

/**
 * Compute the sum a += G*b, where G is the curve's generator point.
 * @return false on failure (such as infinity or zero).
 */
BC_API bool ec_add(ec_compressed& point, const ec_secret& secret);

/**
 * Compute the sum a += G*b, where G is the curve's generator point.
 * @return false on failure (such as infinity or zero).
 */
BC_API bool ec_add(ec_uncompressed& point, const ec_secret& secret);

/**
 * Compute the sum left = (left + right) % n, where n is the curve order.
 * @return false on failure (such as a zero result).
 */
BC_API bool ec_add(ec_secret& left, const ec_secret& right);

/**
 * Compute the product point *= secret.
 * @return false on failure (such as infinity or zero).
 */
BC_API bool ec_multiply(ec_compressed& point, const ec_secret& secret);

/**
 * Compute the product point *= secret.
 * @return false on failure (such as infinity or zero).
 */
BC_API bool ec_multiply(ec_uncompressed& point, const ec_secret& secret);

/**
 * Compute the product left = (left * right) % n, where n is the curve order.
 * @return false on failure (such as a zero result).
 */
BC_API bool ec_multiply(ec_secret& left, const ec_secret& right);

} // namespace libbitcoin

#endif

