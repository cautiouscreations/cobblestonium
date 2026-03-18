# Build

## Linux

```bash
cmake -S handheld/project/linux -B handheld/build/linux-x64 -DCMAKE_BUILD_TYPE=Release
cmake --build handheld/build/linux-x64 -j
```

Output: `handheld/build/linux-x64/cobblestonium`

## Web

```bash
emcmake cmake -S handheld/project/linux -B handheld/build/web -DCMAKE_BUILD_TYPE=Release
cmake --build handheld/build/web -j
```

Output: `handheld/build/web/cobblestonium.html`

## Run Web Locally

```bash
python3 -m http.server 8080 --directory handheld/build/web
```

Open `http://localhost:8080/cobblestonium.html`.
