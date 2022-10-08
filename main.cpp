// A segment is continus blocks that are all hole or not-hole.

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <vector>
#include <unistd.h>
#include <any>

using namespace std;
struct Segment {
    off_t offset;
    off_t size;
    bool isHole;
};

int main(int argc, char *argv[]){
    int file=open(argv[0],O_RDWR);
    struct stat fileSystem;
    stat("/", &fileSystem);
    int blockSize=fileSystem.st_blksize;

    struct stat fileStat;
    fstat(file,&fileStat);

    off_t fileSize=fileStat.st_size;
    auto onSegment= [&](Segment segment){

        fpunchhole_t punchhole;
        punchhole.fp_flags=0x00;
        punchhole.reserved=0x00;
        punchhole.fp_offset=segment.offset;
        punchhole.fp_length=segment.size;

        fcntl(file,F_PUNCHHOLE, &punchhole);
    };
    vector<uint8_t> buffer(blockSize,0x00);
    off_t segmentOffset=0;
    off_t blockOffset=0;
    any isLastBlockHole;

    while (blockOffset<fileSize){
        int readSize=read(file,&buffer[0],blockSize);
        if (readSize < blockSize && readSize < fileSize - blockOffset){
        }
        bool isBlockHole=all_of(buffer.cbegin(),buffer.cbegin()+readSize, [](uint8_t i){return i==0x00;});
        if (isLastBlockHole.has_value()){
            bool unwrappedIsLastBlockHole=any_cast<bool>(isLastBlockHole);
            if (unwrappedIsLastBlockHole!=isBlockHole){
                Segment segment;
                segment.offset=segmentOffset;
                segment.size=blockOffset-segmentOffset;
                segment.isHole=unwrappedIsLastBlockHole;
                onSegment(segment);
                isLastBlockHole=isBlockHole;
                segmentOffset=blockOffset;
            }
        }else{
            isLastBlockHole=isBlockHole;
        }
        blockOffset+=off_t(readSize);
    }
    if (isLastBlockHole.has_value()){
        bool isLastBlockHole=any_cast<bool>(isLastBlockHole);
        Segment segment;
        segment.offset=segmentOffset;
        segment.size=fileSize-segmentOffset;
        segment.isHole=isLastBlockHole;
        onSegment(segment);
    }
}