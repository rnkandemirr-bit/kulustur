# Kulüstür: geliştirme notları (nerede kaldık)

Bu dosya, çalışma yarıda kalırsa kaldığı yerden devam edebilmek için tutuluyor.

## Kullanıcının istekleri (6 Kasım 2026 gecesi)
1. 5 farklı harita/iklim: Güneşli, Çöl, Kar, Yağmur, Şehir.
2. Her haritada 10 bölüm (toplam 50). Bir haritanın 10 bölümü bitince sıradaki harita açılır. 5. harita bitince oyun biter: baştan oynanabilir ya da sonsuz moda geçilir.
3. Sonsuz mod: ölene kadar devam eden yol, belli mesafede iklim değişir. Liderlik tablosu.
4. ClusterTruck tarzı zorluklar: ters kontroller, zorunlu şerit değiştirme, lazerler vb.
5. Sesler: zıplama, koşma, araç, ortam, müzik.
6. Bölüm geçiş ekranı (referans: mobil oyun dünya haritası), bize özgü tasarım.
7. Sadece tır olmasın: farklı araç tipleri ve mekanikler.
8. Paketi sonuna kadar kullan: ray, halat, makara, yay, homing attack, halkalar.
9. Sabaha kadar çalış, bitince kaydet, notları yaz, bilgisayarı kapat.

## Durum (6 Kasım 2026 sabahı)
- [x] 5 dünya: renkler, ışık, sis, hava (yağmur/kar/kum), şimşek, dünyaya özel dekor (ağaç, kaktüs, kum tepesi, çam, direk, gökdelen, neon).
- [x] 50 bölüm (`KulusturMakeLevelConfig`), yıldızlar, kilit açma, kayıt (`UKulusturSave`).
- [x] 9 araç tipi: Tır, DevTır, Otobüs, Kamyonet, Tanker (patlar), YaylıTır (paket yayı), Fırlatıcı, Hoplayan, Zikzak.
- [x] Paket: grind ray (Pulley_BP yardımcısıyla), halat, dikey makara, yay, homing hedefi, altın ve halka modelleri, Niagara efektleri, paket sesleri ve müzikleri.
- [x] Zorluklar: lazer, ters kontrol, rüzgar, buz, gökten araç, sis, şerit duvarı, makas atan araçlar, hızlanma, karanlık.
- [x] Sonsuz mod: parça parça üretilen yol, 1,5 km'de bir dünya değişimi, artan zorluk, liderlik tablosu (ilk 10, isimli).
- [x] Menüler (Canvas): isim girişi, ana menü, dünya kartları, kıvrımlı bölüm yolu + bilgi paneli, liderlik, ayarlar (ses), nasıl oynanır, duraklatma, bölüm sonucu (yıldız animasyonu, yeni dünya açıldı), oyun sonu (baştan / sonsuz), sonsuz mod bitiş ekranı.
- [x] Sesler: üretilen 10 ses (`Tools/ses_uret.py` → `Content/Kulustur/Audio`), paket sesleri (zıplama, iniş, adım, altın, halka, yay, UI), dünya müzikleri.
- [x] Birinci şahıs kamera (KulusturDirector::SetupFirstPerson; paketin kameraları her kare kapatılır), paketin İngilizce UMG yardım yazısı kaldırılır (RemovePackageWidgets), ilk 8 sn Türkçe kontrol paneli.
- [x] Bitiş artık çarpışmasız kapı: yol ve trafik bitişten 300 m sonrasına kadar devam eder, kazanma oyuncunun X konumuna bakar. Bölümler 450 m - 1,3 km (engeller yol boyuna orantılı).
- [x] Test edildi: tüm dünyalar görsel olarak, bölüm akışı (ölüm → yeniden, bitiş → sonuç ekranı → yeni dünya), sonsuz mod, menü ekranları.

## Bilinen eksikler / sonraki adımlar
- Elle (insan) oynanarak denge ayarı yapılmadı: yıldız süreleri, rüzgar gücü, trafik yoğunluğu ayarlanabilir.
- Fırlatıcı rampası otomatik testle doğrulandı (`-KulusturTestLaunch`). Hız halkası ve ters kontrol elle (klavyeyle) denenmedi.
- Paketlenmiş (.exe) sürüm yok; oyun `Oyunu_Baslat.bat` ile editörün oyun modunda açılıyor.
- `GetMovementBase` uyarısı (UE 5.8'de eskimiş): ileride `GetMovementBaseObject` ile değiştirilmeli.

## Paket hakkında öğrenilenler (test edildi)
- Karakter: `Demo/ThirdPersonAssets/Blueprints/ThirdPersonCharacter_BP` (Platformer, Homing Attack, Grind Rail bileşenleri).
- **Grind ray / halat (Spline_Mesh_BP) haritada bir `Pulley_BP` aktörü yoksa çalışmıyor.** Pist kurulurken bir tane doğuruluyor.
- Ray şekli: Blueprint şablonundaki spline noktaları doğurmadan önce değiştiriliyor (`KulusturPackage::SpawnRail`).
- Dikey makara (Zipline_BP): `PulleyLocation` (göreli), `CableLength` (m), `PulleyStartLocation` (dünya), `ForwardLaunchVelocity`, `UpwardsLaunchVelocity`.
- Yay (Jump_Up_Spring_B_BP): `LaunchHeight`.
- Jump pad / Boost ring / Arc spring koddan doğunca tetiklenmedi → kendi fırlatıcı ve halkalarımızı yazdık.
- Homing attack KAPALI: ileri hızı sıfırlayıp oyuncuyu düşürüyordu. Tavana hedef konmuyor, bileşende CanSearchTargets?=False. Yerine kendi çift zıplamamız (TryAirJump). Hareket: TuneMovement (JumpZ 1050, yerçekimi 1.6, hava kontrolü 0.9). Test: `-KulusturTestJump` (+ `-KulusturTestDash`).
- Yetenekler (KulusturDirector::TickMoves): kıyı affı 0.15 sn, zıplama hafızası 0.15 sn, E atılma (TryDash), Q ağır çekim (SetSlowMo, zaman 0.3), Ctrl çakılma (TrySlam), çift zıplamada kamera taklası (FlipT), hızla genişleyen FOV. Enerji 0-100: +7/sn, ağır çekim -38/sn, akrobasi inişi +10/hareket (OnLanded). Ağır çekimde bölüm süresi gerçek zamanla sayılır.

## Test araçları (komut satırı)
- `-KulusturTest`: durumları loglar, `-KulusturTestTime=N` saniye sonra kapanır (varsayılan 40).
- `-KulusturMode=Level|Endless`, `-KulusturWorld=0..4`, `-KulusturLevel=0..9`, `-KulusturScreen=Worlds|Levels|Leaderboard|Settings|Help|GameComplete`.
- `-KulusturShots`: 6/12/18/24. saniyelerde ekran görüntüsü (Saved/Screenshots).
- `-KulusturGod`: ölmez (görüntü için). `-KulusturTestWin`: bitişe ışınlar. `-KulusturNoUI`: arayüzü gizler.
- `-KulusturDump`, `-KulusturPkgTest`: paket inceleme araçları.
- Derleme: `Build.bat kulusturEditor Win64 Development -Project=... -NoUBA -MaxParallelActions=1` (bellek az olduğu için UBA kapalı).
- Sesleri yeniden üretmek: `python Tools/ses_uret.py Tools/Sesler`, sonra `UnrealEditor-Cmd.exe kulustur.uproject -run=pythonscript -script=Tools/ses_aktar.py`.

## Video ve GitHub (6 Kasım 2026)
- `-KulusturAuto`: oyunu kendi oynayan pilot (AutoPilot/PickAutoTarget). `-KulusturFrames=N`: N kare sonra çık. `-KulusturVideoSave`: menü için dolu örnek kayıt.
- Video çekimi: `-benchmark -fps=30 -dumpmovie` ile her kare Saved/Screenshots'a yazılır, ffmpeg ile mp4 yapılır. Çıktılar `Video/` (git dışı).
- Paketleme: DefaultGame.ini'de paket klasörleri her zaman pişirilir (paket sınıfları kodda yolla yükleniyor). Çıktı `Paket/` (git dışı).
- GitHub: `GitHub_a_Yukle.bat` -> Tools/github_yukle.ps1. Geçmişi olmayan `yayin` dalını `main` olarak iter, zip'i v1.0 Release olarak yükler. Fab paketinin ham dosyaları lisans gereği yüklenmez.
