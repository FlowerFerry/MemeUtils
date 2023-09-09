
#ifndef MMUPP_APP_LMDB_ENV_HPP_INCLUDED
#define MMUPP_APP_LMDB_ENV_HPP_INCLUDED

#include <mego/predef/symbol/likely.h>
#include <mego/err/lmdb_convert.h>
#include <liblmdb/lmdb.h>
#include <megopp/err/err.hpp>
#include <megopp/util/scope_cleanup.h>

#include <memory>

namespace mmupp::app {
    
    struct lmdb_env
    {
        lmdb_env() = delete;
        
        inline lmdb_env(::MDB_env* _obj)
            : _obj(_obj)
        {}

        lmdb_env(const lmdb_env&) = delete;
        lmdb_env(lmdb_env&&) = default;
        
        inline ~lmdb_env()
        {
            if (MEGO_SYMBOL__LIKELY(_obj != NULL))
            {
                ::mdb_env_close(_obj);
            }
        }

        lmdb_env& operator=(const lmdb_env&) = delete;
        lmdb_env& operator=(lmdb_env&&) = default;
        
        template<typename _Fn>
        mgpp::err do_read (_Fn&& _fn);

        template<typename _Fn>
        mgpp::err do_write(_Fn&& _fn);

        template<typename _Fn>
        mgpp::err do_write_begin(::MDB_txn*&, ::MDB_dbi&, _Fn&& _fn);

        inline constexpr ::MDB_env* native_handle() const noexcept { return _obj; }

    private:
        ::MDB_env* _obj;
    };
    using lmdb_env_ptr = std::shared_ptr<lmdb_env>;

    template <typename _Fn>
    inline mgpp::err lmdb_env::do_read(_Fn &&_fn)
    {    
        if (MEGO_SYMBOL__UNLIKELY(_obj == nullptr)) {
            return mgpp::err{ MGEC__ERR, "invalid lmdb env" };
        }

        int ec = 0;
        ::MDB_txn* txn = nullptr;
        ec = ::mdb_txn_begin(_obj, nullptr, MDB_RDONLY, &txn);
        if (MEGO_SYMBOL__UNLIKELY(ec != 0)) {
            return mgpp::err{ mgec__from_lmdb_err(ec), "failed to begin lmdb txn" };
        }
        auto txn_cleanup = megopp::util::scope_cleanup__create(
            [&txn]() { ::mdb_txn_abort(txn); });

        ::MDB_dbi dbi = 0;
        ec = ::mdb_dbi_open(txn, nullptr, 0, &dbi);
        if (MEGO_SYMBOL__UNLIKELY(ec != 0)) {
            return mgpp::err{ mgec__from_lmdb_err(ec), "failed to open lmdb dbi" };
        }
        auto dbi_cleanup = megopp::util::scope_cleanup__create(
            [&] { ::mdb_dbi_close(_obj, dbi); });

        auto e = _fn(_obj, txn, dbi);
        if (MEGO_SYMBOL__UNLIKELY(e)) {
            return e;
        }

        return mgpp::err::make_ok();
    }

    template <typename _Fn>
    inline mgpp::err lmdb_env::do_write(_Fn &&_fn)
    {
        ::MDB_txn* txn = nullptr;
        ::MDB_dbi  dbi = 0;
        auto e = do_write_begin(txn, dbi, _fn);
        if (MEGO_SYMBOL__UNLIKELY(e)) {
            return e;
        }
        
        int ec = ::mdb_txn_commit(txn);
        if (MEGO_SYMBOL__UNLIKELY(ec != 0)) {
            return mgpp::err{ mgec__from_lmdb_err(ec), "failed to commit lmdb txn" };
        }

        return mgpp::err::make_ok();
    }

    template <typename _Fn>
    inline mgpp::err lmdb_env::do_write_begin(::MDB_txn*& _txn, ::MDB_dbi& _dbi, _Fn &&_fn)
    {
        if (MEGO_SYMBOL__UNLIKELY(_obj == nullptr)) {
            return mgpp::err{ MGEC__ERR, "invalid lmdb env" };
        }

        int ec = 0;
        ::MDB_txn* txn = nullptr;
        ec = ::mdb_txn_begin(_obj, nullptr, 0, &txn);
        if (MEGO_SYMBOL__UNLIKELY(ec != 0)) {
            return mgpp::err{ mgec__from_lmdb_err(ec), "failed to begin lmdb txn" };
        }
        auto txn_cleanup = megopp::util::scope_cleanup__create(
            [&txn]() { ::mdb_txn_abort(txn); });

        ::MDB_dbi dbi = 0;
        ec = ::mdb_dbi_open(txn, nullptr, 0, &dbi);
        if (MEGO_SYMBOL__UNLIKELY(ec != 0)) {
            return mgpp::err{ mgec__from_lmdb_err(ec), "failed to open lmdb dbi" };
        }
        auto dbi_cleanup = megopp::util::scope_cleanup__create(
            [&]() { ::mdb_dbi_close(_obj, dbi); });

        auto e = _fn(_obj, txn, dbi);
        if (MEGO_SYMBOL__UNLIKELY(e)) {
            return e;
        }

        txn_cleanup.cancel();
        dbi_cleanup.cancel();

        _txn = txn;
        _dbi = dbi;
        return mgpp::err::make_ok();
    }
}

#endif // !MMUPP_APP_LMDB_ENV_HPP_INCLUDED
