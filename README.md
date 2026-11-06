# Kulüstür

**Tırdan tıra atla, yere değme!**

Kulüstür, otoyolda giden tırların üstünde zıplayarak bitişe ulaşmaya çalıştığın birinci şahıs bir aksiyon oyunu.
*ClusterTruck*'tan esinlenerek, yapay zekâ **Claude** (Anthropic) ile **Unreal Engine 5.8**'de baştan yapıldı.
"Eski oyunları Claude'a tekrar yaptırıyorum" YouTube serisinin oyunudur.

## İndir ve oyna

1. Sağ taraftaki **Releases** bölümünden en son sürümü aç ve **`Kulustur_v1.0_Windows.zip`** dosyasını indir.
2. Zip'i bir klasöre çıkar.
3. **`Kulustur.exe`**'ye çift tıkla.

Gereken: Windows 10/11 (64 bit), DirectX 12 destekli bir ekran kartı.
Windows "bilinmeyen yayımcı" uyarısı gösterirse **Ek bilgi → Yine de çalıştır**'a bas.

## Kontroller

| Tuş | Ne yapar |
|---|---|
| WASD | Koş / yön değiştir |
| Fare | Etrafa bak |
| Boşluk | Zıpla. Havada bir daha bas: çift zıplama (takla) |
| Shift | Depar |
| E | Atılma: gittiğin yöne fırla (enerji harcar) |
| Q (basılı tut) | Ağır çekim (enerji harcar) |
| Ctrl / C | Havadan aşağı çakıl |
| Sol / sağ tık | Rayda yön değiştir / halatta in-çık |
| R | Bölümü yeniden başlat |
| Esc / P | Duraklat |

Havada takla, atılma ya da çakılma yapıp bir aracın üstüne inersen **akrobasi** sayılır ve enerjin dolar.

## İçerik

- **5 dünya × 10 bölüm**: Güneşli Otoyol, Kavurucu Çöl, Karlı Dağlar, Fırtınalı Vadi, Neon Şehir
- **Sonsuz mod** ve liderlik tablosu: her 1,5 km'de iklim değişir, oyun gittikçe zorlaşır
- **Farklı araçlar**: tanker (patlar), yaylı tır, fırlatıcı, hoplayan araç, zikzakçı...
- **Zorluklar**: lazerler, ters kontroller, rüzgar, buz, gökten araç yağması, sis, karanlık...
- Her bölümde 3 yıldız: bitir, yıldız süresini geç, altınların yarısını topla

## Kaynaktan derlemek isteyenler için

- Unreal Engine 5.8 gerekir. Kod `Source/kulustur/` klasöründedir (C++).
- Oyun, Fab'daki **3D Platformer Game Bundle** paketinin parçalarını kullanır. Paketin ham dosyaları lisansı gereği bu depoda **yoktur**; derlemek için paketi Fab'dan kendi hesabınla projeye ekleyip `Content/3DPlatformerGameBundle/` klasörüne koyman gerekir. İndirilebilir sürümde (Releases) her şey hazırdır.

## Not

Bu bir hayran yapımı / esinlenme projesidir. ClusterTruck, Landfall Games'in oyunudur; bu projenin Landfall Games ile bir bağlantısı yoktur.
