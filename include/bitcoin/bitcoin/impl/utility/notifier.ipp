/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_NOTIFIER_IPP
#define LIBBITCOIN_NOTIFIER_IPP

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <bitcoin/bitcoin/compat.hpp>
#include <bitcoin/bitcoin/utility/asio.hpp>
#include <bitcoin/bitcoin/utility/assert.hpp>
#include <bitcoin/bitcoin/utility/dispatcher.hpp>
#include <bitcoin/bitcoin/utility/thread.hpp>
#include <bitcoin/bitcoin/utility/threadpool.hpp>
////#include <bitcoin/bitcoin/utility/track.hpp>

namespace libbitcoin {

template <typename Key, typename... Args>
notifier<Key, Args...>::notifier(threadpool& pool,
    const std::string& class_name)
  : limit_(0), stopped_(true), dispatch_(pool, class_name)
    /*, track<notifier<Key, Args...>>(class_name)*/
{
}

template <typename Key, typename... Args>
notifier<Key, Args...>::notifier(threadpool& pool, size_t limit,
    const std::string& class_name)
  : limit_(limit), stopped_(true), dispatch_(pool, class_name)
    /*, track<notifier<Key, Args...>>(class_name)*/
{
}

template <typename Key, typename... Args>
notifier<Key, Args...>::~notifier()
{
    BITCOIN_ASSERT_MSG(subscriptions_.empty(), "notifier not cleared");
}

template <typename Key, typename... Args>
void notifier<Key, Args...>::start()
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    subscribe_mutex_.lock_upgrade();

    if (stopped_)
    {
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        subscribe_mutex_.unlock_upgrade_and_lock();
        stopped_ = false;
        subscribe_mutex_.unlock();
        //---------------------------------------------------------------------
        return;
    }

    subscribe_mutex_.unlock_upgrade();
    ///////////////////////////////////////////////////////////////////////////
}

template <typename Key, typename... Args>
void notifier<Key, Args...>::stop()
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    subscribe_mutex_.lock_upgrade();

    if (!stopped_)
    {
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        subscribe_mutex_.unlock_upgrade_and_lock();
        stopped_ = true;
        subscribe_mutex_.unlock();
        //---------------------------------------------------------------------
        return;
    }

    subscribe_mutex_.unlock_upgrade();
    ///////////////////////////////////////////////////////////////////////////
}

template <typename Key, typename... Args>
void notifier<Key, Args...>::subscribe(handler&& notify, const Key& key,
    const asio::duration& duration, Args... stopped_args)
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    subscribe_mutex_.lock_upgrade();

    if (!stopped_)
    {
        const auto it = subscriptions_.find(key);

        if (it != subscriptions_.end())
        {
            // Do not make const as that voids the move.
            auto expires = asio::steady_clock::now() + duration;
            //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            subscribe_mutex_.unlock_upgrade_and_lock();
            it->second.expires = std::move(expires);
            subscribe_mutex_.unlock();
            //---------------------------------------------------------------------
            return;
        }
        else if (limit_ == 0 || subscriptions_.size() < limit_)
        {
            auto value = notifier<Key, Args...>::value
            {
                std::forward<handler>(notify),
                asio::steady_clock::now() + duration
            };

            // Do not make const as that voids the move.
            auto pair = std::make_pair(key, std::move(value));
            //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            subscribe_mutex_.unlock_upgrade_and_lock();
            subscriptions_.emplace(std::move(pair));
            subscribe_mutex_.unlock();
            //---------------------------------------------------------------------
            return;
        }
    }

    subscribe_mutex_.unlock_upgrade();
    ///////////////////////////////////////////////////////////////////////////

    // Limit exceeded and stopped share the same return arguments.
    notify(stopped_args...);
}

template <typename Key, typename... Args>
void notifier<Key, Args...>::unsubscribe(const Key& key,
    Args... unsubscribed_args)
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    subscribe_mutex_.lock_upgrade();

    if (!stopped_)
    {
        const auto it = subscriptions_.find(key);

        if (it != subscriptions_.end())
        {
            const auto handler = it->second.notify;

            //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            subscribe_mutex_.unlock_upgrade_and_lock();
            subscriptions_.erase(it);
            subscribe_mutex_.unlock();
            //-----------------------------------------------------------------
            handler(unsubscribed_args...);
            return;
        }
    }

    subscribe_mutex_.unlock_upgrade();
    ///////////////////////////////////////////////////////////////////////////
}

template <typename Key, typename... Args>
void notifier<Key, Args...>::purge(Args... expired_args)
{
    const auto now = asio::steady_clock::now();

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    subscribe_mutex_.lock();

    // Move subscribers from the member map to a temporary map.
    map subscriptions;
    std::swap(subscriptions, subscriptions_);

    subscribe_mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////

    // Subscriptions may be created while this loop is executing.
    // Invoke and discard expired subscribers from temporary map.
    for (const auto& entry: subscriptions)
    {
        if (now > entry.second.expires)
        {
            entry.second.notify(expired_args...);
            continue;
        }

        // Critical Section
        ///////////////////////////////////////////////////////////////////
        unique_lock lock(subscribe_mutex_);
        subscriptions_.emplace(entry);
        ///////////////////////////////////////////////////////////////////
    }
}

template <typename Key, typename... Args>
void notifier<Key, Args...>::invoke(Args... args)
{
    do_invoke(args...);
}

template <typename Key, typename... Args>
void notifier<Key, Args...>::relay(Args... args)
{
    // This enqueues work while maintaining order.
    dispatch_.ordered(&notifier<Key, Args...>::do_invoke,
        this->shared_from_this(), args...);
}

// private
template <typename Key, typename... Args>
void notifier<Key, Args...>::do_invoke(Args... args)
{
    // Critical Section (prevent concurrent handler execution)
    ///////////////////////////////////////////////////////////////////////////
    unique_lock lock(invoke_mutex_);

    // Critical Section (protect stop)
    ///////////////////////////////////////////////////////////////////////////
    subscribe_mutex_.lock();

    // Move subscribers from the member map to a temporary map.
    map subscriptions;
    std::swap(subscriptions, subscriptions_);

    subscribe_mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////

    // Subscriptions may be created while this loop is executing.
    // Invoke subscribers from temporary map and resubscribe as indicated.
    for (const auto& entry: subscriptions)
    {
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // DEADLOCK RISK, notify must not return to invoke.
        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        if (entry.second.notify(args...))
        {
            // Critical Section
            ///////////////////////////////////////////////////////////////////
            subscribe_mutex_.lock_upgrade();

            if (stopped_)
            {
                subscribe_mutex_.unlock_upgrade();
                //-------------------------------------------------------------
                continue;
            }

            subscribe_mutex_.unlock_upgrade_and_lock();
            //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
            subscriptions_.emplace(entry);

            subscribe_mutex_.unlock();
            ///////////////////////////////////////////////////////////////////
        }
    }

    ///////////////////////////////////////////////////////////////////////////
}

} // namespace libbitcoin

#endif
