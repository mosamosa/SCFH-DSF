# How to Access Shared Memory

※データにアクセスする場合は必ずそれぞれのミューテックスで排他処理をすること

1. `SCFH_DSF_GMUTEX` のミューテックスを開いてロック
2. `SCFH_DSF_PIDS` の共有メモリを開いてSCFH DSFがロードされているPIDを取得
3. `SCFH_DSF_GMUTEX` のミューテックスのロックを開放
4. `SCFH_DSF_MUTEX{PID}` のミューテックスを開いてロック (例 PID:123の場合は `SCFH_DSF_MUTEX123`)
5. `SCFH_DSF_AREA{PID}` の共有メモリを開いてアクセス (例 PID:123の場合は `SCFH_DSF_AREA123`)
6. `SCFH_DSF_MUTEX{PID}` のミューテックスのロックを開放

# Constants

```cpp
#define RESIZE_MODE_SW_NEAREST 0  // ソフトウェア縮小 (最近傍補間)
#define RESIZE_MODE_SW_BILINEAR 1 // ソフトウェア縮小 (バイリニア補完)
#define RESIZE_MODE_DD_1PASS 2    // DirectDraw縮小 1Pass (Vista以降では使用禁止)
#define RESIZE_MODE_DD_2PASS 3    // DirectDraw縮小 2Pass (Vista以降では使用禁止)
```

# Structures

```cpp
#pragma pack(push, 1)

struct RECTF
{
    float left, top, right, bottom;
};

struct AreaInfo
{
    BOOL active;     // 出力範囲有効化
    DWORD hwnd;      // ウィンドウのハンドル
    RECT src;        // 取込範囲 ピクセル単位 ウィンドウの右上を[0, 0]とした座標系
    RECTF dst;       // 出力範囲 右上端を[0.0, 0.0]、右下端を[1.0, 1.0]とした座標系
    bool zoom;       // 取込範囲が出力範囲より小さい場合に拡大するか否か
    bool fixAspect;  // 取込範囲と出力範囲のアスペクトが異なる場合にアスペクト比を調整するか否か
    bool showCursor; // マウスカーソルを取り込むか否か
    bool showLW;     // レイヤードウィンドウを取り込むか否か
};

struct SharedInfo
{
    BOOL active;             // [inout] SCFH DSF有効化 (プロセスからアンロードされたときにFALSEに設定される)
    struct AreaInfo area[8]; // [in] 取込範囲設定
    POINT dummy1;            // 未使用
    int resizeMode;          // [in] 縮小モード (RESIZE_MODE_*)
    int dummy2;              // 未使用
    double aveTime;          // [out] 一フレームの平均処理時間
    int width, height;       // [out] 出力サイズ
    double framerate;        // [out] 出力フレームレート
    DWORD threadNum;         // [in] ソフトウェア縮小スレッド数
    BOOL overSampling;       // [in] オーバーサンプリング有効化
};

struct ProcessInfo
{
    DWORD pid;      // [out] プロセスID
    char name[260]; // [out] 実行ファイル名
};

#pragma pack(pop)
```

# Mutex

## `SCFH_DSF_GMUTEX`

- `SCFH_DSF_PIDS` へのアクセス時に使用
- レジストリへのアクセス時に使用

## `SCFH_DSF_MUTEX{PID}`

- `SCFH_DSF_AREA{PID}` へのアクセス時に使用

# Shared Memory

## `SCFH_DSF_PIDS`

※アクセスする場合は必ず `SCFH_DSF_GMUTEX` で排他処理すること

- SCFH DSFがロードしているプロセスリスト
- Size: `sizeof(ProcessInfo) * 32`

## `SCFH_DSF_AREA{PID}`

※アクセスする場合は必ず `SCFH_DSF_MUTEX{PID}` で排他処理すること

- SCFH DSFの設定データ
- Size: `sizeof(SharedInfo)`

# Registry

## `HKCU\SOFTWARE\SCFH DSF\{EXE filename}`

※アクセスする場合は必ず `SCFH_DSF_GMUTEX` で排他処理すること

- `Framerate` (REG_SZ): `Float`
- `Width` (REG_SZ): `Integer`
- `Height` (REG_SZ): `Integer`
