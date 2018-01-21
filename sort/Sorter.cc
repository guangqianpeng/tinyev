//
// Created by frank on 17-9-23.
//

#include <cstdint>
#include <cassert>
#include <unistd.h>

#include <tinyev/Logger.h>
#include <tinyev/EventLoop.h>
#include <tinyev/TcpConnection.h>
#include <tinyev/TcpServer.h>
#include <tinyev/ThreadPool.h>

using namespace ev;

class File: noncopyable
{
protected:
    File(const char* name, const char* mode)
            : name_(name),
              file_(::fopen(name, mode))
    {
        if (file_ == nullptr)
            SYSFATAL("fopen()");
    }
    ~File() { ::fclose(file_); }

public:
    void rewind()
    { ::rewind(file_); }
    const char* name() const
    { return name_.c_str(); }
    bool eof() const
    { return feof_unlocked(file_) != 0; }
    void unlink()
    { ::unlink(name_.c_str()); }
    void flush()
    { ::fflush_unlocked(file_); }

public:
    virtual void readBatch(std::vector<int64_t>& vec, size_t maxSize) = 0;
    virtual bool readOne(int64_t* x) = 0;
    virtual void writeBatch(const std::vector<int64_t>& vec) = 0;
    virtual void writeOne(int64_t x) = 0;

protected:
    const std::string name_;
    FILE* file_;
};

class FileOfBinary: public File
{
public:
    FileOfBinary(const char* name, const char* mode)
            : File(name, mode)
    {}

    void readBatch(std::vector<int64_t>& vec, size_t maxSize) override
    {
        assert(!eof());
        vec.resize(maxSize);
        size_t n = fread_unlocked(vec.data(), sizeof(int64_t), maxSize, file_);
        if (n < maxSize) {
            if (!eof())
                SYSFATAL("fread()");
            vec.resize(n);
        }
    }

    bool readOne(int64_t* x) override
    {
        assert(!eof());
        size_t n = fread_unlocked(x, sizeof(int64_t), 1, file_);
        if (n != 1) {
            if (!eof())
                SYSFATAL("fread()");
            return false;
        }
        return true;
    }

    void writeBatch(const std::vector<int64_t>& vec) override
    {
        size_t n = fwrite_unlocked(vec.data(), sizeof(int64_t), vec.size(), file_);
        if (n != vec.size())
            SYSFATAL("fwrite()");
    }

    void writeOne(int64_t x) override
    {
        size_t n = fwrite_unlocked(&x, sizeof(int64_t), 1, file_);
        if (n != 1)
            SYSFATAL("fwrite()");
    }

    int64_t at(size_t index) const
    {
        int ret = fseek(file_, sizeof(int64_t) * index, SEEK_SET);
        if (ret == -1)
            SYSFATAL("fseek()");

        int64_t elem;
        size_t n = fread_unlocked(&elem, sizeof(int64_t), 1, file_);
        if (n != 1)
            FATAL("fread()");
        return elem;
    }
};

class FileOfText: public File
{
public:
    FileOfText(const char* name, const char* mode)
            :File(name, mode)
    {}


    void readBatch(std::vector<int64_t>& vec, size_t maxSize) override
    {
        vec.clear();
        vec.reserve(maxSize);

        char buf[32];
        size_t i = 0;
        for (; i < maxSize; ++i) {
            char* ret = fgets_unlocked(buf, 32, file_);
            if (ret == nullptr)
                break;
            int64_t x = toIntOrDie(buf);
            vec.push_back(x);
        }

        if (i < maxSize) {
            if (!eof())
                SYSFATAL("fgets()");
            vec.shrink_to_fit();
        }
    }

    bool readOne(int64_t* x) override
    {
        char buf[32];
        char* ret = fgets_unlocked(buf, 32, file_);
        if (ret == nullptr) {
            if (!eof())
                SYSFATAL("fgets()");
            return false;
        }
        *x = toIntOrDie(buf);
        return true;
    }

    void writeBatch(const std::vector<int64_t>& vec) override
    {
        char buf[32];
        for(int64_t x: vec) {
            toString(x, buf);
            int ret = fputs_unlocked(buf, file_);
            assert(ret >= 0); (void)ret;
        }
    }

    void writeOne(int64_t x) override
    {
        char buf[32];
        char* p = toString(x, buf);
        *p++ = '\n';
        *p = '\0';
        int ret = fputs_unlocked(buf, file_);
        assert(ret >= 0); (void)ret;
    }

private:
    static char* toString(int64_t x, char* buf)
    {
        static const char digits[19] = {
                '9', '8', '7', '6', '5', '4', '3', '2', '1', '0',
                '1', '2', '3', '4', '5', '6', '7', '8', '9'};
        static const char* zero = &digits[9];

        bool negative = (x < 0);
        char* p = buf;
        do {
            // lsd - least significant digit
            auto lsd = static_cast<int>(x % 10);
            x /= 10;
            *p++ = (zero[lsd]);
        } while (x != 0);

        if (negative)
            *p++ = '-';
        *p = '\0';
        std::reverse(buf, p);

        return p;
    }

    static int64_t toIntOrDie(const char* buf)
    {
        char* end;
        errno = 0;
        int64_t ret = strtol(buf, &end, 10);
        if (buf == end || errno == ERANGE)
            FATAL("strtol()");
        return ret;
    }
};

class Sorter: noncopyable
{
public:
    Sorter(const char* inputFileName, File* output)
            : input_(inputFileName, "r"),
              output_(output),
              sorted_(false)
    {}

    void sort(size_t batchSize)
    {
        assert(!sorted_);
        INFO("input '%s', output '%s', batchSize %lu",
              input_.name(), output_->name(), batchSize);
        INFO("split and sort...", blocks_.size());
        splitAndSortBlocks(batchSize);
        INFO("merge %lu blocks...", blocks_.size());
        mergeBlocks();
        INFO("done");
        sorted_ = true;
    }

private:
    typedef FileOfBinary Block;
    typedef std::unique_ptr<FileOfBinary> BlockPtr;
    typedef std::vector<BlockPtr> BlockPtrList;

    void splitAndSortBlocks(size_t batchSize)
    {
        std::vector<int64_t> vec;

        for (int i = 0; ; i++) {

            INFO("proccess block %d...", i);
            INFO("  read block %d...", i);
            // fixme: strtol is cpu bounded
            input_.readBatch(vec, batchSize);
            if (vec.empty())
                break;

            INFO("  sort block %d...", i);
            std::sort(vec.begin(), vec.end());

            char name[32];
            snprintf(name, 32, "%s-shard-%04d", input_.name(), i);

            INFO("  write block %d...");
            auto block = new Block(name, "wb+");
            block->writeBatch(vec);
            block->flush();
            block->rewind();
            block->unlink();

            blocks_.emplace_back(block);

            INFO("[%d/%lu] blocks done", i + 1, blocks_.size());

            if (vec.size() < batchSize)
                break;
        }
    }

    void mergeBlocks()
    {
        std::vector<Record> records;
        for (auto& b: blocks_) {
            int64_t elem;
            bool success =  b->readOne(&elem);
            assert(success); (void)success;
            records.push_back({elem, b.get()});
        }

        std::make_heap(records.begin(), records.end());
        while (!records.empty()) {
            std::pop_heap(records.begin(), records.end());
            auto& r = records.back();
            output_->writeOne(r.elem);
            if (r.next())
                std::push_heap(records.begin(), records.end());
            else
                records.pop_back();
        }
    }

private:
    struct Record
    {
        int64_t elem;
        Block* block;

        bool next()
        { return block->readOne(&elem); }
        bool operator<(const Record& rhs) const
        { return elem > rhs.elem; }
    };

    FileOfText input_;
    File* output_;
    BlockPtrList blocks_;
    bool sorted_;
};

int main(int argc, char** argv)
{
    if (argc != 4) {
        printf("usage: ./sort input output batchSize\n");
        return 1;
    }

    size_t batchSize = strtoul(argv[3], nullptr, 10);
    FileOfBinary output(argv[2], "wb");
    Sorter sorter(argv[1], &output);
    sorter.sort(batchSize);
}