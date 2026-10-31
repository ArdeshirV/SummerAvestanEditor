# ویرایشگر اوستایی

[English](README.md)

<table style="border: none; border-collapse: collapse;"  border="0" cellspacing="0" cellpadding="0">
  <tr>
    <td style="border: none; padding: 0;"><img src="https://github.com/ArdeshirV/SummerAvestanEditor/blob/main/img/Icon.png" width="256"></td>
    <td style="border: none; padding: 0 0 0 8px;">ویرایشگر اوستایی یک ویرایشگر متن یونیکد چندسکویی برای خط‌های باستانی و دیگر
نظام‌های نوشتاریست. صفحه‌کلید نرم‌افزاری آن با
اوستایی، پهلوی، پارتی و فارسی باستان آغاز می‌شود و سپس خط‌های تاریخی دیگر، همانند سغدی، خوارزمی، مانوی، هیروگیلیف مصر باستان و
خط‌های دیگر یونیکد، نمادهای عمومی و صفحه‌کلید همهٔ نویسه‌های چاپ‌پذیر یونیکد
را ارائه می‌کند.
</td>
  </tr>
</table>

این برنامه روی لینوکس، ویندوز و اندروید اجرا می‌شود. می‌توانید پرونده‌های متنی
یونیکد را با کدگذاری UTF-8 (با یا بدون BOM)، UTF-16 LE/BE یا UTF-32 LE/BE دارای
BOM باز کنید، ویرایش کنید و ذخیره کنید. داده‌های UTF نامعتبر رد می‌شوند و بی‌سروصدا
جایگزین نخواهند شد. تنظیمات، داده‌های زبان، آوانویسی‌ها، نام‌های نویسه‌های یونیکد
و رشته‌های قابل‌مشاهده برای کاربر از پوشهٔ `assets/` و از طریق منابع Qt در برنامه
جاسازی شده‌اند.

### ویرایش‌گر اوستایی در محیط لینوکس در حال اجرا:
<div dir="rtl">
  <img alt="Summer Avestan Editor V 1.0 on Linux Desktop" src="img/SummerAvestanEditor.png">
</div>


## اجرا و ساخت

به یک محیط توسعهٔ Qt 6 با Qt Widgets و Qt Test نیاز است (`qmake6`). در Debian و
Ubuntu بسته‌های `qt6-base-dev` و `qt6-base-dev-tools` را نصب کنید.

پروندهٔ `Makefile` هدف‌های بومی و چندسکویی را می‌سازد. نام فایل خروجی به شکل
`<نام‌برنامه>-<سیستم‌عامل>-<معماری>` است؛ برای نمونه
`SummerAvestanEditor-Linux-amd64`، `SummerAvestanEditor-Termux-arm64`،
`SummerAvestanEditor-Windows-amd64.exe` و `SummerAvestanEditor-Android-arm64.apk`.

```sh
make                 # ساخت بومی (لینوکس روی میزکار، Termux در Termux)
make linux           # فایل اجرایی لینوکس میزکار
make termux          # فایل اجرایی Termux (اندروید/bionic)
make windows         # فایل exe ویندوز
make android         # بستهٔ apk اندروید
make test            # اجرای آزمون‌های رگرسیون Qt Test
make run             # اجرای ساخت بومی
make clean           # پاک‌سازی dist/ و ساخت‌های آزمون
```

هدف‌های ویژهٔ معماری عبارت‌اند از `linux-amd64 linux-arm64 linux-386`,
`termux-arm64 termux-arm arm termux-amd64`, `windows-amd64 windows-arm64
windows-386` و `android-arm64 android-arm android-amd64`. ساخت‌های متقابل به یک
کیت Qt برای سیستم مقصد نیاز دارند؛ بنابراین متغیر مربوطه را به qmake آن کیت
اشاره دهید:

```sh
python3 scripts/create-windows-icon.py
make windows WINDOWS_QMAKE=/opt/Qt/6.8.0/mingw_64/bin/qmake
make android ANDROID_QMAKE=$HOME/Qt/6.8.0/android_arm64_v8a/bin/qmake \
             ANDROID_NDK_ROOT=$HOME/Android/Sdk/ndk/27.0.12077973
```

پروژهٔ qmake ویندوز، `img/Icon.ico` را به‌عنوان نقشک فایل اجرایی جاسازی می‌کند.
برای ساخت دوبارهٔ آن از `img/Icon.png`، اسکریپت بالا را اجرا کنید؛ این کار به libpng
نیاز دارد.

ساخت تک‌پرونده‌ای با qmake همچنان در دسترس است:
`qmake6 AvestanEditor.pro && make` (یا `make build`)؛ و
`bash tests/run-tests.sh` آزمون‌ها را اجرا می‌کند. یک پروندهٔ CMake نیز برای
انتقال‌پذیری فراهم شده است. برخی بسته‌های Qt در Termux فرادادهٔ ناقص افزونه‌های
Qt را همراه دارند (یک هدف GTK3 آویزان)، و همین باعث می‌شود `find_package(Qt6)`
پیش از کامپایل شکست بخورد؛ مسیر ساخت معتبر و کارآمد همان پروژهٔ qmake است.
بسته‌بندی اندروید از روند معمول Qt for Android
(`androiddeployqt` همراه با Android SDK/NDK) پیروی می‌کند.


## لطفاً کمک مالی کنید!

از هرگونه کمک مالی‌ای که بتوانید به پروژهٔ ما اهدا کنید، صمیمانه سپاسگزاریم.

![کد QR کمک مالی اتریوم](assets/ethereum-donation.png)

Ethereum `0x6E6465394D14975956cd1BD37ab4E35F2C60300E`

Copyright&copy; 2026 [ArdeshirV](mailto:ArdeshirV@protonmail.com), Licensed under GPL<sup>v3+</sup>
