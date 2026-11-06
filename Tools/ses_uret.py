# Kulustur icin ortam ve efekt seslerini sentezler (WAV, 22050 Hz, mono).
# Kullanim: python ses_uret.py <cikis_klasoru>
import math, random, struct, sys, wave, os

SR = 22050
random.seed(42)


def write(path, samples):
    peak = max(1e-6, max(abs(s) for s in samples))
    g = 0.89 / peak
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(b"".join(struct.pack("<h", int(max(-1, min(1, s * g)) * 32767)) for s in samples))


def noise(n):
    return [random.uniform(-1, 1) for _ in range(n)]


def lowpass(x, cutoff):
    a = 1 - math.exp(-2 * math.pi * cutoff / SR)
    y, out = 0.0, []
    for s in x:
        y += a * (s - y)
        out.append(y)
    return out


def highpass(x, cutoff):
    lp = lowpass(x, cutoff)
    return [a - b for a, b in zip(x, lp)]


def loopify(x, fade):
    # Sonu basa yumusakca bindir: kesintisiz dongu
    n = len(x) - fade
    out = x[:n]
    for i in range(fade):
        t = i / fade
        out[i] = out[i] * t + x[n + i] * (1 - t)
    return out


def lfo(n, rate, depth, base=1.0, phase=0.0):
    return [base + depth * math.sin(2 * math.pi * rate * i / SR + phase) for i in range(n)]


def ruzgar():
    n = SR * 7
    x = lowpass(noise(n), 500)
    x = lowpass(x, 900)
    m1 = lfo(n, 0.17, 0.45, 0.6)
    m2 = lfo(n, 0.43, 0.2, 0.0, 1.3)
    hiss = highpass(noise(n), 2500)
    y = [x[i] * (m1[i] + m2[i]) + hiss[i] * 0.05 * m1[i] for i in range(n)]
    return loopify(y, SR)


def yagmur():
    n = SR * 5
    x = highpass(noise(n), 1200)
    x = lowpass(x, 6000)
    y = [s * 0.5 for s in x]
    # damla tikirtilari
    for _ in range(900):
        p = random.randrange(n - 400)
        amp = random.uniform(0.2, 0.9)
        f = random.uniform(1500, 4500)
        for k in range(300):
            y[p + k] += amp * math.exp(-k / 40) * math.sin(2 * math.pi * f * k / SR)
    low = lowpass(noise(n), 200)
    y = [y[i] + low[i] * 1.5 for i in range(n)]
    return loopify(y, SR // 2)


def sehir():
    n = SR * 8
    rumble = lowpass(lowpass(noise(n), 120), 160)
    mid = lowpass(highpass(noise(n), 300), 1500)
    m = lfo(n, 0.11, 0.3, 0.8)
    y = [rumble[i] * 3 * m[i] + mid[i] * 0.25 for i in range(n)]
    # uzak korna ve siren
    for _ in range(3):
        p = random.randrange(n - SR)
        f = random.choice([330, 392, 440])
        for k in range(int(SR * 0.6)):
            env = min(1, k / 800) * min(1, (SR * 0.6 - k) / 2000)
            y[p + k] += 0.08 * env * (1 if math.sin(2 * math.pi * f * k / SR) > 0 else -1)
    p = random.randrange(n - SR * 3)
    for k in range(SR * 3):
        f = 700 + 250 * math.sin(2 * math.pi * 0.7 * k / SR)
        y[p + k] += 0.05 * math.sin(2 * math.pi * f * k / SR) * math.sin(math.pi * k / (SR * 3))
    return loopify(lowpass(y, 3000), SR)


def ugultu():
    # Trafik / motor ugultusu
    n = SR * 5
    y = [0.0] * n
    nz = lowpass(noise(n), 180)
    ph = [0.0, 0.0, 0.0]
    for i in range(n):
        wob = 1 + 0.03 * math.sin(2 * math.pi * 0.8 * i / SR)
        fs = [42 * wob, 84 * wob, 127 * wob]
        s = 0
        for j, f in enumerate(fs):
            ph[j] += 2 * math.pi * f / SR
            s += math.sin(ph[j]) * [1, 0.5, 0.25][j]
        y[i] = s * 0.5 + nz[i] * 2.5
    return loopify(lowpass(y, 700), SR // 2)


def lazer():
    n = SR * 3
    y = []
    for i in range(n):
        t = i / SR
        saw = 2 * ((110 * t) % 1) - 1
        s = saw * 0.5 + math.sin(2 * math.pi * 220 * t) * 0.3 + math.sin(2 * math.pi * 1320 * t) * 0.08
        s *= 0.75 + 0.25 * math.sin(2 * math.pi * 6 * t)
        y.append(s)
    return loopify(lowpass(y, 2500), SR // 4)


def carpisma():
    n = int(SR * 1.4)
    nz = noise(n)
    y = [0.0] * n
    for i in range(n):
        t = i / SR
        y[i] = nz[i] * math.exp(-t * 6) + math.sin(2 * math.pi * 55 * t) * math.exp(-t * 8) * 1.5
    for _ in range(14):
        f = random.uniform(300, 2600)
        d = random.uniform(4, 12)
        st = random.randrange(int(SR * 0.15))
        a = random.uniform(0.1, 0.35)
        for i in range(st, n):
            t = (i - st) / SR
            y[i] += a * math.sin(2 * math.pi * f * t) * math.exp(-t * d)
    return lowpass(y, 5000)


def patlama():
    n = int(SR * 3)
    nz = lowpass(noise(n), 900)
    y = []
    for i in range(n):
        t = i / SR
        boom = math.sin(2 * math.pi * (60 - 25 * min(t, 1)) * t) * math.exp(-t * 2.2) * 2
        y.append(nz[i] * 3 * math.exp(-t * 1.6) * min(1, t * 200) + boom)
    crack = highpass(noise(int(SR * 0.2)), 1500)
    for i, s in enumerate(crack):
        y[i] += s * math.exp(-i / (SR * 0.05))
    return lowpass(y, 3500)


def gok():
    n = int(SR * 5)
    nz = lowpass(lowpass(noise(n), 250), 300)
    y = []
    for i in range(n):
        t = i / SR
        env = min(1, t * 6) * math.exp(-t * 0.8) * (0.7 + 0.3 * math.sin(2 * math.pi * 1.7 * t) * math.sin(2 * math.pi * 0.6 * t))
        y.append(nz[i] * 6 * env)
    crack = highpass(noise(int(SR * 0.4)), 800)
    for i, s in enumerate(crack):
        y[i] += s * 0.6 * math.exp(-i / (SR * 0.08))
    return y


def korna():
    n = int(SR * 0.75)
    y = []
    for i in range(n):
        t = i / SR
        env = min(1, t * 60) * min(1, (0.75 - t) * 20)
        s = 0
        for f in (311, 392):
            s += 1 if math.sin(2 * math.pi * f * t) > 0 else -1
        y.append(s * env * 0.5)
    return lowpass(lowpass(y, 1800), 2500)


def uyari():
    n = int(SR * 0.7)
    y = []
    for i in range(n):
        t = i / SR
        f = 880 if (t % 0.35) < 0.175 else 660
        env = min(1, (t % 0.175) * 200) * min(1, (0.175 - (t % 0.175)) * 200)
        y.append(math.sin(2 * math.pi * f * t) * env)
    return y


SOUNDS = {
    "Ruzgar": ruzgar, "Yagmur": yagmur, "Sehir": sehir, "Ugultu": ugultu, "Lazer": lazer,
    "Carpisma": carpisma, "Patlama": patlama, "Gok": gok, "Korna": korna, "Uyari": uyari,
}

if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "."
    os.makedirs(out, exist_ok=True)
    for name, fn in SOUNDS.items():
        write(os.path.join(out, name + ".wav"), fn())
        print("yazildi", name)
