# Kulüstür

ClusterTruck tarzı bir oyun: yolda giden araçların üstünde zıplayarak BİTİŞ kapısından geç. Yere değersen ölürsün. Oyun birinci şahıs (FPS) görünümdedir; bitiş kapısı uzaktadır (bölüme göre 450 m - 1,3 km) ve araçlar kapıdan geçip yollarına devam eder.

## Başlatmak

**Oyunu_Baslat.bat** dosyasına çift tıkla. İlk açılışta sürücü adını sorar (liderlik tablosunda bu isim görünür).

## Kontroller

- **WASD**: koş / yön değiştir
- **Fare**: kamera
- **Boşluk**: zıpla (yaklaşık 3,5 m). Havada bir kez daha basınca çift zıplama (takla); ileri hızın korunur. Kenardan düştükten hemen sonra da zıplayabilirsin.
- **E**: atılma - gittiğin yöne fırla (30 enerji, havada bir kez).
- **Q (basılı tut)**: ağır çekim (enerji harcar).
- **Ctrl / C**: havadan aşağı çakıl.
- **Akrobasi**: havada takla/atılma/çakılma yapıp bir aracın üstüne inersen enerji kazanırsın. Enerji zamanla da dolar.
- **Shift**: depar
- **R**: bölümü yeniden başlat
- **Esc / P**: duraklat (ses ayarları, yeniden başla, harita, menü)
- Menülerde fare ile tıkla ya da ok tuşları ve Enter kullan.

## Oyun modları

### Dünyalar (5 dünya x 10 bölüm)
1. **Güneşli Otoyol**: ısınma turu, yeşil tepeler.
2. **Kavurucu Çöl**: kum fırtınası, rüzgar, patlayan tankerler.
3. **Karlı Dağlar**: kar yağışı, kaygan tavanlar, lazerler.
4. **Fırtınalı Vadi**: sağanak yağmur, şimşek, ters dönen kontroller.
5. **Neon Şehir**: gün batımında gökdelenler, hepsi bir arada.

Bir dünyanın 10 bölümünü bitirince sonraki dünya açılır. Her bölümde 3 yıldız var:
1 yıldız bitirmek, 2. yıldız "yıldız süresi"nin altında bitirmek, 3. yıldız altınların yarısını toplamak.
5. dünyanın son bölümü bitince oyun biter: baştan oynayabilir ya da sonsuz moda geçebilirsin.

### Sonsuz mod
Bitişi olmayan yol. Ölene kadar devam eder, her 1,5 km'de dünya (iklim) değişir ve oyun gittikçe zorlaşır.
En uzağa gidenler **Liderlik Tablosu**'na yazılır (ilk 10).

## Araçlar
- **Tır, dev tır (çift römork), otobüs, kamyonet**: farklı boy ve yükseklikler.
- **Tanker**: çarpışınca yarım saniye sonra patlar, yakındakileri savurur.
- **Yaylı tır** (sarı-siyah şeritli): tavanındaki yay seni çok yükseğe fırlatır.
- **Fırlatıcı** (yeşil rampalı): rampaya basınca ileri fırlatır.
- **Hoplayan araç** (sarı süspansiyonlu): ara ara zıplar.
- **Zikzakçı** (siyah çizgili): sürekli şerit değiştirir.

## Paket parçaları (Fab: 3D Platformer Game Bundle)
- **Grind rayları** (araçların üstünde kıvrılan raylar): üstüne in, kay. Üstünde altınlar var.
- **Halatlar** (iki direk arası): tutun, kayarak ilerle.
- **Makaralar** (sarı kuleler): tutun, yukarı çekip ileri fırlatır.
- **Hız halkaları**, **altınlar**, homing attack hedefleri, efektler ve sesler.

## Zorluklar (bölüme göre değişir)
Lazer kapıları (üstünden atla), ters kontroller, rüzgar, buz, gökten araç yağması, yoğun sis,
şerit duvarı (tek açık şerit: oraya geçmek zorundasın), ani şerit değiştiren araçlar, trafiğin aniden hızlanması, karanlık.

## Projenin yapısı
- `Source/kulustur/`: oyunun kodu (C++)
  - `KulusturDirector`: oyunun yöneticisi (menü, bölüm, sonsuz mod, zorluklar, sesler, kayıt).
  - `KulusturTrack`: pist (yol, dekor, engeller, raylar, halatlar, makaralar, halkalar, altınlar).
  - `KulusturTraffic`: araç trafiği (doğma, şerit değiştirme, çarpışma, patlama).
  - `KulusturVehicle`: araç tipleri.
  - `KulusturEnvironment`: hava (güneş, sis, yağmur/kar/kum, şimşek).
  - `KulusturTypes`: 5 dünyanın görünümü ve 50 bölümün ayarları.
  - `KulusturSave`: ilerleme, yıldızlar, rekorlar, liderlik tablosu, ayarlar.
  - `KulusturHUD`: tüm arayüz ve menüler.
- `Content/Kulustur/Audio/`: üretilen ortam ve efekt sesleri (rüzgar, yağmur, şehir, motor, lazer, çarpışma, patlama, gök gürültüsü, korna, uyarı).
- `Tools/ses_uret.py`, `Tools/ses_aktar.py`: bu sesleri üreten ve Unreal'a aktaran betikler.
- Kayıt dosyası: `Saved/SaveGames/Kulustur.sav` (silersen ilerleme sıfırlanır).

Kodu değiştirdikten sonra projeyi yeniden derlemek gerekir. Bunun için Claude'a "derle" demen yeterli.
