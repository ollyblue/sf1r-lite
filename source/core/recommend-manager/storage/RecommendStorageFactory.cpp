#include "RecommendStorageFactory.h"
#include "LocalUserManager.h"
#include "RemoteUserManager.h"
#include "LocalPurchaseManager.h"
#include "RemotePurchaseManager.h"
#include "LocalVisitManager.h"
#include "RemoteVisitManager.h"
#include "LocalCartManager.h"
#include "RemoteCartManager.h"
#include "LocalRateManager.h"
#include "RemoteRateManager.h"
#include "LocalEventManager.h"
#include "RemoteEventManager.h"

#include <configuration-manager/CassandraStorageConfig.h>
#include <log-manager/CassandraConnection.h>

#include <boost/filesystem.hpp>

namespace bfs = boost::filesystem;

namespace sf1r
{

RecommendStorageFactory::RecommendStorageFactory(
    const CassandraStorageConfig& cassandraConfig,
    const std::string& collection,
    const std::string& dataDir
)
    : cassandraConfig_(cassandraConfig)
    , cassandraClient_(NULL)
    , storagePaths_(STORAGE_PATH_ID_NUM)
{
    if (cassandraConfig_.enable)
    {
        initRemoteStorage_(collection);
    }
    else
    {
        initLocalStorage_(dataDir);
    }
}

void RecommendStorageFactory::initRemoteStorage_(const std::string& collection)
{
    storagePaths_[STORAGE_PATH_ID_USER] = collection + "_users";
    storagePaths_[STORAGE_PATH_ID_PURCHASE] = collection + "_purchase";
    storagePaths_[STORAGE_PATH_ID_VISIT_ITEM] = collection + "_visit_item";
    storagePaths_[STORAGE_PATH_ID_VISIT_RECOMMEND] = collection + "_visit_recommend";
    storagePaths_[STORAGE_PATH_ID_VISIT_SESSION] = collection + "_visit_session";
    storagePaths_[STORAGE_PATH_ID_CART] = collection + "_cart";
    storagePaths_[STORAGE_PATH_ID_RATE] = collection + "_rate";
    storagePaths_[STORAGE_PATH_ID_EVENT] = collection + "_event";

    const std::string& keyspace = cassandraConfig_.keyspace;
    cassandraClient_ = CassandraConnection::instance().getCassandraClient(keyspace);
    if (! cassandraClient_)
    {
        LOG(ERROR) << "failed to connect cassandra server, keyspace: " << keyspace;
    }
}

void RecommendStorageFactory::initLocalStorage_(const std::string& dataDir)
{
    bfs::path dataPath(dataDir);
    bfs::path userDir = dataPath / "user";
    bfs::path eventDir = dataPath / "event";

    bfs::create_directory(userDir);
    bfs::create_directory(eventDir);

    storagePaths_[STORAGE_PATH_ID_USER] = (userDir / "user.db").string();
    storagePaths_[STORAGE_PATH_ID_PURCHASE] = (eventDir / "purchase.db").string();
    storagePaths_[STORAGE_PATH_ID_VISIT_ITEM] = (eventDir / "visit_item.db").string();
    storagePaths_[STORAGE_PATH_ID_VISIT_RECOMMEND] = (eventDir / "visit_recommend.db").string();
    storagePaths_[STORAGE_PATH_ID_VISIT_SESSION] = (eventDir / "visit_session.db").string();
    storagePaths_[STORAGE_PATH_ID_CART] = (eventDir / "cart.db").string();
    storagePaths_[STORAGE_PATH_ID_RATE] = (eventDir / "rate.db").string();
    storagePaths_[STORAGE_PATH_ID_EVENT] = (eventDir / "event.db").string();
}

UserManager* RecommendStorageFactory::createUserManager() const
{
    const std::string& path = storagePaths_[STORAGE_PATH_ID_USER];

    if (cassandraConfig_.enable)
        return new RemoteUserManager(cassandraConfig_.keyspace, path, cassandraClient_);

    return new LocalUserManager(path);
}

PurchaseManager* RecommendStorageFactory::createPurchaseManager() const
{
    const std::string& path = storagePaths_[STORAGE_PATH_ID_PURCHASE];

    if (cassandraConfig_.enable)
        return new RemotePurchaseManager(cassandraConfig_.keyspace, path, cassandraClient_);

    return new LocalPurchaseManager(path);
}

VisitManager* RecommendStorageFactory::createVisitManager() const
{
    const std::string& itemPath = storagePaths_[STORAGE_PATH_ID_VISIT_ITEM];
    const std::string& recommendPath = storagePaths_[STORAGE_PATH_ID_VISIT_RECOMMEND];
    const std::string& sessionPath = storagePaths_[STORAGE_PATH_ID_VISIT_SESSION];

    if (cassandraConfig_.enable)
    {
        return new RemoteVisitManager(cassandraConfig_.keyspace,
                                      itemPath, recommendPath, sessionPath,
                                      cassandraClient_);
    }

    return new LocalVisitManager(itemPath, recommendPath, sessionPath);
}

CartManager* RecommendStorageFactory::createCartManager() const
{
    const std::string& path = storagePaths_[STORAGE_PATH_ID_CART];

    if (cassandraConfig_.enable)
        return new RemoteCartManager(cassandraConfig_.keyspace, path, cassandraClient_);

    return new LocalCartManager(path);
}

RateManager* RecommendStorageFactory::createRateManager() const
{
    const std::string& path = storagePaths_[STORAGE_PATH_ID_RATE];

    if (cassandraConfig_.enable)
        return new RemoteRateManager(cassandraConfig_.keyspace, path, cassandraClient_);

    return new LocalRateManager(path);
}

EventManager* RecommendStorageFactory::createEventManager() const
{
    const std::string& path = storagePaths_[STORAGE_PATH_ID_EVENT];

    if (cassandraConfig_.enable)
        return new RemoteEventManager(cassandraConfig_.keyspace, path, cassandraClient_);

    return new LocalEventManager(path);
}

} // namespace sf1r