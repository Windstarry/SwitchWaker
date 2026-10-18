#ifndef JUTCACHEFONT_H
#define JUTCACHEFONT_H

#include "JSystem/JUtility/JUTResFont.h"
#include "global.h"

class JKRAramBlock;

class JUTCacheFont : public JUTResFont {
public:
    struct TGlyphCacheInfo {
        /* 0x0 */ TGlyphCacheInfo* mPrev;
        /* 0x4 */ TGlyphCacheInfo* mNext;
        /* 0x08 */ u16 field_0x8;
        /* 0x0A */ u16 field_0xa;
        /* 0x0C */ u16 field_0xc;
        /* 0x0E */ u16 field_0xe;
        /* 0x10 */ u32 field_0x10;
        /* 0x14 */ u16 mTexFormat;
        /* 0x16 */ u16 field_0x16;
        /* 0x18 */ u16 field_0x18;
        /* 0x1A */ u16 mWidth;
        /* 0x1C */ u16 mHeight;
        /* 0x1E */ u16 field_0x1e;
    };

    struct TCachePage : TGlyphCacheInfo {
        /* 0x20 */ GXTexObj mTexObj;
    };  // Size: 0x40 (GameCube; 0x48 on the host, where the inherited mPrev/mNext pointers are 8 bytes)

    enum EPagingType {
        PAGE_TYPE_0,
        PAGE_TYPE_1,
    };

    JUTCacheFont(ResFONT const*, u32, JKRHeap*);
    void deleteMemBlocks_CacheFont();
    void initialize_state();
    int getMemorySize(ResFONT const*, u16*, u32*, u16*, u32*, u16*, u32*, u32*);
    int initiate(ResFONT const*, void*, u32, JKRHeap*);
    bool internal_initiate(ResFONT const*, void*, u32, JKRHeap*);
    bool allocArea(void*, u32, JKRHeap*);
    bool allocArray(JKRHeap*);
    TGlyphCacheInfo* determineBlankPage();
    void getGlyphFromAram(TGlyphCacheInfo*, TCachePage*, int*, int*);
    TCachePage* loadCache_char_subroutine(int*, bool);
    void invalidiateAllCache();
    void unlink(TGlyphCacheInfo*);
    void prepend(TGlyphCacheInfo*);

    virtual ~JUTCacheFont();
    virtual void loadImage(int, GXTexMapID);
    virtual void setBlock();

    void setPagingType(EPagingType type) { mPagingType = type; }
    GXTexObj* getTexObj(void* buffer) { return &((TCachePage*)buffer)->mTexObj; }
    void delete_and_initialize() { deleteMemBlocks_CacheFont(); initialize_state(); }

    // The 0x40 in the decomp is the GameCube's sizeof(TCachePage); the host's is 0x48, and the cache
    // size has to match the page stride allocArea derives from it (mDoExt_initFontCommon computes the
    // same thing inline).
    static u32 calcCacheSize(u32 param_0, int param_1) { return (ALIGN_NEXT(param_0, 0x20) + getTexAreaOffset()) * param_1; }

    // The texture area follows the page header (the decomp's pCachePage + 1). The GameCube's
    // 0x40-byte header is 32-byte aligned by construction, but the host's 0x48 is not, and the ARAM
    // DMA asserts 32-byte alignment on both ends (JKRAramPiece::orderAsync), so round the header up.
    static u32 getTexAreaOffset() { return ALIGN_NEXT(sizeof(TCachePage), 0x20); }

private:
    /* 0x70 */ u32 mTotalWidSize;
    /* 0x74 */ u32 mTotalGlySize;
    /* 0x78 */ u32 mTotalMapSize;
    /* 0x7C */ u8* field_0x7c;
    /* 0x80 */ u8* field_0x80;
    /* 0x84 */ u8* field_0x84;
    /* 0x88 */ u32 mMaxSheetSize;
    /* 0x8C */ EPagingType mPagingType;
    /* 0x90 */ u8* mCacheBuffer;
    /* 0x94 */ int field_0x94;
    /* 0x98 */ u32 mCachePage;
    /* 0x9C */ TGlyphCacheInfo* field_0x9c;
    /* 0xA0 */ TGlyphCacheInfo* field_0xa0;
    /* 0xA4 */ TGlyphCacheInfo* field_0xa4;
    /* 0xA8 */ u32 field_0xa8;
    /* 0xAC */ JKRAramBlock* field_0xac;
    /* 0xB0 */ u8 field_0xb0;
    /* 0xB4 */ int field_0xb4;
};  // Size: 0xB8

#endif /* JUTCACHEFONT_H */
