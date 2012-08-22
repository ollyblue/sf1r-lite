/*!
 \file      AutoFillChildManager.h
 \author    Hongliang.Zhao&&Qiang.Wang
 \brief     AutoFillChildManager will get the search item list which match the prefix of the keyword user typed.
 \dat	    2012-07-26
*/

#if !defined(_AUTO_FILL_SUBMANAGER_)
#define _AUTO_FILL_SUBMANAGER_

#include "word_leveldb_table.hpp"

#include <boost/tuple/tuple.hpp>
#include <boost/thread/thread.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/filesystem.hpp>

#include <util/ThreadModel.h>
#include <util/ustring/UString.h>
#include <mining-manager/query-correction-submanager/QueryCorrectionSubmanager.h>
#include <util/filesystem.h>
#include <util/functional.h>
#include <log-manager/LogAnalysis.h>
#include <util/singleton.h>
#include <idmlib/util/directory_switcher.h>
#include <am/leveldb/Table.h>
#include <am/succinct/wat_array/wat_array.hpp>
//#include <am/trie/b_trie.hpp>

#include <process/common/XmlConfigParser.h>

#include <am/vsynonym/QueryNormalize.h>
#include <am/range/AmIterator.h>
#include <ir/id_manager/IDManager.h>
#include <log-manager/LogManager.h>
#include <log-manager/UserQuery.h>
#include <util/cronexpression.h>

#include <vector>
#include <list>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <ctime>
#include <string.h>
#include <assert.h>
#include <signal.h>

namespace sf1r
{

#define UINT32_LEN sizeof(uint32_t)
#define FREQ_TYPE_LEN sizeof(uint32_t)
#define HITNUM_LEN sizeof(uint32_t)
#define TOPN_LEN sizeof(uint32_t)

class  WordLevelDBTable;

class AutoFillChildManager: public boost::noncopyable
{
    typedef uint32_t FREQ_TYPE;
    typedef boost::tuple<uint32_t, uint32_t, izenelib::util::UString> ItemValueType;
    typedef std::pair<uint32_t, izenelib::util::UString> PropertyLabelType;
    typedef izenelib::ir::idmanager::AutoFillIDManager IDManger;

    bool fromSCD_;
    bool isUpdating_;
    bool isUpdating_Wat_;
    bool isIniting_;

    uint32_t updatelogdays_;
    uint32_t alllogdays_;
    uint32_t topN_;

    uint32_t updateMin_;
    uint32_t updateHour_;
    string collectionName_;
    string AutofillPath_;
    string leveldbPath_;
    string SCDDIC_;
    //string BtriePath_;
    string ItemPath_;
    string ItemdbPath_;
    fstream out;// for log
    std::vector<string> SCDHaveDone;
    string SCDLogPath_;

    std::string cronJobName_;
    boost::thread updateThread_;
     boost::mutex buildCollectionMutex_;
    izenelib::util::CronExpression cronExpression_;

    struct ItemType
    {
        uint64_t offset_;
        std::string strItem_;
        inline bool operator > (const ItemType other) const
        {
            return strItem_ > other.strItem_ ;
        }

        inline bool operator < (const ItemType other) const
        {
            return strItem_ < other.strItem_;
        }
        inline bool operator > (const std::string& other)
        {
            return strItem_ > other;
        }

        inline bool operator < (const std::string& other)
        {
            return strItem_ < other;
        }

        inline bool operator == (const std::string& other)
        {
            return strItem_ == other;
        }
        inline void operator = (const ItemType& other)
        {
            offset_=other.offset_;
            strItem_=other.strItem_;
        }

        inline bool operator == (const ItemType& other)
        {
            return  (strItem_==other.strItem_);
        }

        inline bool operator != (const ItemType& other)
        {
            return  (strItem_!=other.strItem_);
        }
    };

    struct ValueType
    {
        uint32_t size_;
        std::string  strValue_;
        FREQ_TYPE freq_;
        uint32_t HitNum_;

        void toValueType(const std::string& value, FREQ_TYPE freq, uint32_t HitNum)
        {
            size_ = value.length() + UINT32_LEN*2 + FREQ_TYPE_LEN;
            strValue_ = value;
            freq_ = freq;
            HitNum_ = HitNum;
        }

        void toString(std::string &str)
        {
            char *str_ = new char[size_];
            *(uint32_t*)str_ = size_;
            for(uint32_t i = UINT32_LEN; i < size_ - FREQ_TYPE_LEN - UINT32_LEN; i++)
                str_[i] = strValue_[i - UINT32_LEN];
            *(FREQ_TYPE*)(str_ + size_ - FREQ_TYPE_LEN - UINT32_LEN) = freq_;
            *(uint32_t*)(str_ + size_- UINT32_LEN) = HitNum_;
            string s(str_, size_);
            str = s;
            delete [] str_;
        }

        void getValueType(const char* str)
        {
            size_ = *(uint32_t*)str;
            strValue_.insert(0, str + UINT32_LEN, size_ - UINT32_LEN*2 - FREQ_TYPE_LEN);
            freq_ = *(FREQ_TYPE*)(str + size_ - FREQ_TYPE_LEN - UINT32_LEN);
            HitNum_ = *(uint32_t*)(str + size_ - UINT32_LEN);
        }

        inline void getHitString(std::string &str)
        {
            str = boost::lexical_cast<string>(HitNum_) + "/" + strValue_;
        }
    };
    QueryNormalize* QN_;
    WordLevelDBTable dbTable_;
    WordLevelDBTable dbItem_;
    wat_array::WatArray wa_;
    std::vector<ItemType> ItemVector_;
    IDManger *idManager_;

public:
    AutoFillChildManager();

    ~AutoFillChildManager();

    bool Init(const std::string& fillSupportPath, const std::string& collectionName,  const string& cronExpression);
    bool InitWhileHaveLeveldb();
    bool RealInit();
    bool InitFromSCD();
    bool InitFromLog();
    void SaveItem();
    void LoadItem();
    bool openDB(string path, string path2);
    void closeDB();

    bool buildDbIndex(const std::list<QueryType>& queryList);

    void buildItemList(std::string key);
    void buildItemVector();
    void buildTopNDbTable(std::string &value, const uint32_t offset);
    bool getOffset(const std::string& query , uint64_t& OffsetStart, uint64_t& OffsetEnd);


    bool getAutoFillListFromOffset(uint64_t OffsetStart, uint64_t OffsetEnd, std::vector<std::pair<izenelib::util::UString,uint32_t> >& list);
    bool getAutoFillListFromWat(const izenelib::util::UString& query, std::vector<std::pair<izenelib::util::UString,uint32_t> >& list);
    bool getAutoFillListFromDbTable(const izenelib::util::UString& query, std::vector<std::pair<izenelib::util::UString,uint32_t> >& list);

    void buildWat_array(bool _fromleveldb);
    void updateAutoFill();
    void setSource(bool fromSCD);

    bool getAutoFillList(const izenelib::util::UString& query, std::vector<std::pair<izenelib::util::UString,uint32_t> >& list);
    bool buildIndex(const std::list<ItemValueType>& queryList, const std::list<PropertyLabelType>& labelList);
    void updateFromLog();
    void updateFromSCD();
    void SaveSCDLog();
    void LoadSCDLog();
public:
    static std::string system_resource_path_;
};
} // end - namespace sf1r
#endif // _AUTO_FILL_SUBMANAGER_