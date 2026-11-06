# Kulustur'u GitHub'a yukler: herkese acik depo + indirilebilir oyun (Release).
# GitHub_a_Yukle.bat ile calistir. Tek yapman gereken tarayicida GitHub'a giris yapmak.
$ErrorActionPreference = "Continue"
$proj = Split-Path -Parent $PSScriptRoot
Set-Location $proj
$repoName = "kulustur"
$zip = Join-Path $proj "Paket\Kulustur_v1.0_Windows.zip"

function Adim($t) { Write-Host ""; Write-Host "==> $t" -ForegroundColor Cyan }

# 1) GitHub araci (gh)
Adim "GitHub araci kontrol ediliyor"
$gh = (Get-Command gh -ErrorAction SilentlyContinue).Source
if (-not $gh) {
    Write-Host "GitHub araci kuruluyor (winget)..."
    winget install --id GitHub.cli -e --accept-source-agreements --accept-package-agreements | Out-Host
    $gh = "C:\Program Files\GitHub CLI\gh.exe"
}
if (-not (Test-Path $gh) -and -not (Get-Command gh -ErrorAction SilentlyContinue)) { throw "gh kurulamadi. https://cli.github.com adresinden elle kurup tekrar dene." }

# 2) Giris
Adim "GitHub girisi"
& $gh auth status 2>$null | Out-Null
if ($LASTEXITCODE -ne 0) {
    Write-Host "Tarayici acilacak: ekranda cikan kodu girip GitHub hesabinla onayla."
    & $gh auth login --hostname github.com --git-protocol https --web
    if ($LASTEXITCODE -ne 0) { throw "Giris yapilamadi." }
}
& $gh auth setup-git | Out-Null
$user = (& $gh api user --jq .login).Trim()
Write-Host "Giris yapildi: $user"

# 3) Gecmisi olmayan temiz bir yayin dali (yerel calisma dosyalarina dokunmaz)
Adim "Yayin surumu hazirlaniyor"
$sha = (git commit-tree "HEAD^{tree}" -m "Kulustur v1.0").Trim()
git branch -f yayin $sha | Out-Null

# 4) Depo
Adim "Depo olusturuluyor: $user/$repoName"
& $gh repo view "$user/$repoName" 2>$null | Out-Null
if ($LASTEXITCODE -ne 0) {
    & $gh repo create "$user/$repoName" --public --description "Tirdan tira atla, yere degme! Claude ile Unreal Engine 5.8'de yapilan oyun." | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "Depo olusturulamadi." }
}
$url = "https://github.com/$user/$repoName"
git push --force "$url.git" yayin:main | Out-Host
if ($LASTEXITCODE -ne 0) { throw "Kod yuklenemedi." }

# 5) Indirilebilir oyun
Adim "Oyun dosyasi yukleniyor (birkac dakika surebilir)"
if (-not (Test-Path $zip)) { throw "Oyun paketi bulunamadi: $zip" }
& $gh release view v1.0 --repo "$user/$repoName" 2>$null | Out-Null
if ($LASTEXITCODE -ne 0) {
    & $gh release create v1.0 $zip --repo "$user/$repoName" --title "Kulustur v1.0" --notes-file (Join-Path $proj "Tools\surum_notlari.md") | Out-Host
} else {
    & $gh release upload v1.0 $zip --repo "$user/$repoName" --clobber | Out-Host
}
if ($LASTEXITCODE -ne 0) { throw "Oyun dosyasi yuklenemedi." }

$indir = "$url/releases/latest"
Set-Content -Path (Join-Path $proj "Video\github_baglantisi.txt") -Value "Depo: $url`r`nIndirme: $indir" -Encoding UTF8
Adim "TAMAM!"
Write-Host "Depo:    $url" -ForegroundColor Green
Write-Host "Indirme: $indir" -ForegroundColor Green
Write-Host "Bu baglantilar Video\github_baglantisi.txt dosyasina da yazildi. Video aciklamasina ekleyebilirsin."
Start-Process $url
