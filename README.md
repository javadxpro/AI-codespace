# KIMIA

موتور بازی C++17 و ویرایشگر گزینه‌محور KIMIA World.

## ساخت و تست

```bash
cmake -B build -DKIMIA_WERROR=ON
cmake --build build -j4
ctest --test-dir build --output-on-failure
```

مستندات در `Documentation/` و نقشهٔ راه در `ROADMAP.md` قرار دارد.
