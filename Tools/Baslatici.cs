// Kulustur baslatici: oyunu editor acmadan, dogrudan tam ekran oyun modunda acar.
// Derleme: csc.exe /target:winexe /out:Kulustur.exe Baslatici.cs
using System;
using System.Diagnostics;
using System.IO;
using System.Windows.Forms;

static class Baslatici
{
    [STAThread]
    static void Main(string[] args)
    {
        string proje = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.DesktopDirectory), "kulustur", "kulustur.uproject");
        string motor = @"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe";

        // Exe proje klasorune tasinirsa oradaki projeyi kullan
        string yanindaki = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "kulustur.uproject");
        if (File.Exists(yanindaki)) proje = yanindaki;

        if (!File.Exists(proje))
        {
            MessageBox.Show("Oyun projesi bulunamadi:\n" + proje, "Kulustur", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return;
        }
        if (!File.Exists(motor))
        {
            MessageBox.Show("Unreal Engine 5.8 bulunamadi:\n" + motor, "Kulustur", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return;
        }

        // "pencere" argumani verilirse pencerede ac
        bool pencere = args.Length > 0 && args[0].ToLowerInvariant().Contains("pencere");
        string ekran = pencere ? "-windowed -ResX=1600 -ResY=900" : "-fullscreen";

        var bilgi = new ProcessStartInfo
        {
            FileName = motor,
            Arguments = "\"" + proje + "\" -game " + ekran + " -nosplash",
            UseShellExecute = false,
            WorkingDirectory = Path.GetDirectoryName(proje)
        };
        Process.Start(bilgi);
    }
}
