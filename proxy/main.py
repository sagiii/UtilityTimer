import functions_framework
import requests
import struct
import random
import io
from PIL import Image

WIKI_APIS = {
    "minecraft": "https://minecraft.wiki/w/api.php",
    "terraria":  "https://terraria.wiki.gg/api.php",
    "stardew":   "https://stardewvalleywiki.com/mediawiki/api.php",
    "factorio":  "https://wiki.factorio.com/api.php",
    "pokemon":   "https://bulbapedia.bulbagarden.net/w/api.php",
}

TARGET_W, TARGET_H = 240, 133

@functions_framework.http
def get_image(request):
    topic = request.args.get("topic", "minecraft").lower()
    sub   = request.args.get("sub",   "mob").lower()

    api_url = WIKI_APIS.get(topic, WIKI_APIS["minecraft"])

    # WikiMedia allimages API でサブトピックに合う画像を検索
    try:
        resp = requests.get(api_url, params={
            "action":  "query",
            "list":    "allimages",
            "aisearch": sub,
            "ailimit": "50",
            "format":  "json",
            "aiprop":  "url|size|mime",
        }, timeout=10, headers={"User-Agent": "UtilityTimer/1.0"})
        resp.raise_for_status()
        images = resp.json().get("query", {}).get("allimages", [])
    except Exception:
        return "upstream error", 502

    # PNG のみに絞り込み、サイズが大きすぎるものを除外
    images = [
        i for i in images
        if i.get("mime") == "image/png" and i.get("size", 0) < 2_000_000
    ]
    if not images:
        return "no images found", 404

    # ランダムに 1 枚選択
    img_url = random.choice(images)["url"]

    try:
        img_resp = requests.get(img_url, timeout=15,
                                headers={"User-Agent": "UtilityTimer/1.0"})
        img_resp.raise_for_status()
        img_data = img_resp.content
    except Exception:
        return "image download failed", 502

    # 画像処理: RGBA → 黒背景合成 → 240×133 リサイズ
    try:
        img = Image.open(io.BytesIO(img_data)).convert("RGBA")
        bg = Image.new("RGB", img.size, (0, 0, 0))
        bg.paste(img, mask=img.split()[3])
        img = bg.resize((TARGET_W, TARGET_H), Image.LANCZOS)
    except Exception:
        return "image processing failed", 500

    # RGB565 リトルエンディアンに変換
    out = bytearray()
    for r, g, b in img.getdata():
        val = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)
        out += struct.pack("<H", val)

    return (
        bytes(out),
        200,
        {
            "Content-Type":   "application/octet-stream",
            "Content-Length": str(len(out)),
            "Cache-Control":  "public, max-age=86400",
        },
    )
