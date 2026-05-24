import functions_framework
import requests
import struct
import random
import io
from PIL import Image

TARGET_W, TARGET_H = 240, 133
HEADERS = {"User-Agent": "UtilityTimer/1.0"}

# Minecraft Fandom: categorymembers → imageinfo の2ステップ
# sub → カテゴリ名のマッピング
MC_CATEGORIES = {
    "mob":    "Mob_images",
    "block":  "Mob_textures",
    "item":   "Mob_images",
    "biome":  "Mob_images",
}

def get_minecraft_images():
    """Minecraft Fandom から Mob_images カテゴリのファイル一覧 + URL を返す"""
    api = "https://minecraft.fandom.com/api.php"

    # Step1: カテゴリメンバー取得
    r = requests.get(api, params={
        "action": "query", "list": "categorymembers",
        "cmtitle": "Category:Mob_images", "cmtype": "file",
        "cmlimit": "50", "format": "json",
    }, headers=HEADERS, timeout=10)
    r.raise_for_status()
    members = r.json().get("query", {}).get("categorymembers", [])
    if not members:
        return []

    # Step2: imageinfo で実際の URL を取得（最大50件をバッチ処理）
    titles = "|".join(m["title"] for m in members[:50])
    r2 = requests.get(api, params={
        "action": "query", "titles": titles,
        "prop": "imageinfo", "iiprop": "url|mime|size",
        "format": "json",
    }, headers=HEADERS, timeout=10)
    r2.raise_for_status()
    pages = r2.json().get("query", {}).get("pages", {})

    images = []
    for page in pages.values():
        ii = page.get("imageinfo", [{}])[0]
        if ii.get("mime") == "image/png" and 5000 < ii.get("size", 0) < 2_000_000:
            images.append(ii["url"])
    return images


def get_terraria_images(sub):
    """Terraria wiki から aifrom でサブトピック起点の PNG 一覧を返す"""
    api = "https://terraria.wiki.gg/api.php"
    r = requests.get(api, params={
        "action": "query", "list": "allimages",
        "aifrom": sub.capitalize(), "ailimit": "50",
        "format": "json", "aiprop": "url|mime|size",
    }, headers=HEADERS, timeout=10)
    r.raise_for_status()
    imgs = r.json().get("query", {}).get("allimages", [])
    return [
        i["url"] for i in imgs
        if i.get("mime") == "image/png" and 500 < i.get("size", 0) < 500_000
    ]


def get_generic_images(api_url, sub):
    """汎用 MediaWiki: aisearch でファイルを検索"""
    r = requests.get(api_url, params={
        "action": "query", "list": "allimages",
        "aisearch": sub, "ailimit": "50",
        "format": "json", "aiprop": "url|mime|size",
    }, headers=HEADERS, timeout=10)
    r.raise_for_status()
    imgs = r.json().get("query", {}).get("allimages", [])
    return [
        i["url"] for i in imgs
        if i.get("mime") == "image/png" and 500 < i.get("size", 0) < 2_000_000
    ]


@functions_framework.http
def get_image(request):
    topic = request.args.get("topic", "minecraft").lower()
    sub   = request.args.get("sub",   "mob").lower()

    try:
        if topic == "minecraft":
            urls = get_minecraft_images()
        elif topic == "terraria":
            urls = get_terraria_images(sub)
        elif topic == "stardew":
            urls = get_generic_images("https://stardewvalleywiki.com/mediawiki/api.php", sub)
        elif topic == "factorio":
            urls = get_generic_images("https://wiki.factorio.com/api.php", sub)
        elif topic == "pokemon":
            urls = get_generic_images("https://bulbapedia.bulbagarden.net/w/api.php", sub)
        else:
            urls = get_minecraft_images()
    except Exception as e:
        return f"upstream error: {e}", 502

    if not urls:
        return f"no images found (topic={topic}, sub={sub})", 404

    img_url = random.choice(urls)

    try:
        img_resp = requests.get(img_url, timeout=15, headers=HEADERS)
        img_resp.raise_for_status()
        img_data = img_resp.content
    except Exception as e:
        return f"image download failed: {img_url} / {e}", 502

    try:
        img = Image.open(io.BytesIO(img_data)).convert("RGBA")
        bg = Image.new("RGB", img.size, (0, 0, 0))
        bg.paste(img, mask=img.split()[3])
        img = bg.resize((TARGET_W, TARGET_H), Image.LANCZOS)
    except Exception as e:
        return f"image processing failed: {e}", 500

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
