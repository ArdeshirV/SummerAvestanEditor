package com.example.saf;

import android.content.ClipData;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.UriPermission;
import android.database.Cursor;
import android.net.Uri;
import android.os.Build;
import android.os.ParcelFileDescriptor;
import android.provider.DocumentsContract;
import android.provider.OpenableColumns;
import android.util.Log;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;

/**
 * Thin static wrapper around Android's Storage Access Framework (SAF).
 *
 * Design rules (so the JNI side stays trivial):
 *  - every method is static
 *  - only String / String[] / int / long / boolean / Intent / Context cross the boundary
 *  - no method throws: failures return null / -1 / false / empty array
 *
 * URIs are passed around as Strings (content://...).
 */
public final class SafHelper {

    private static final String TAG = "SafHelper";
    private static final int PERSIST_FLAGS =
            Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION;

    private SafHelper() {}

    // ---------------------------------------------------------------------
    // 1) Intent builders  (C++ starts these with startActivity + request code)
    // ---------------------------------------------------------------------

    /** Pick one or more existing files. mimeTypes may be null/empty (= any file). */
    public static Intent buildOpenDocumentIntent(String[] mimeTypes, boolean allowMultiple) {
        Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        i.addCategory(Intent.CATEGORY_OPENABLE);
        if (mimeTypes == null || mimeTypes.length == 0) {
            i.setType("*/*");
        } else if (mimeTypes.length == 1) {
            i.setType(mimeTypes[0]);
        } else {
            i.setType("*/*");
            i.putExtra(Intent.EXTRA_MIME_TYPES, mimeTypes);
        }
        i.putExtra(Intent.EXTRA_ALLOW_MULTIPLE, allowMultiple);
        i.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION
                | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
        return i;
    }

    /**
     * DO NOT use this for a real "Save As".
     * ACTION_CREATE_DOCUMENT always creates a NEW document and the provider silently
     * renames it ("note (1).txt") when the name already exists. The app is never told
     * that a collision happened and can never overwrite. Use buildPickFolderIntent() +
     * resolveSaveTarget() + createFileExact() below instead.
     * Only suitable for "Export a copy" where auto-numbering is acceptable.
     */
    public static Intent buildCreateDocumentIntent(String mimeType, String suggestedName) {
        Intent i = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        i.addCategory(Intent.CATEGORY_OPENABLE);
        i.setType((mimeType == null || mimeType.isEmpty()) ? "application/octet-stream" : mimeType);
        if (suggestedName != null && !suggestedName.isEmpty()) {
            i.putExtra(Intent.EXTRA_TITLE, suggestedName);
        }
        i.addFlags(PERSIST_FLAGS | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION);
        return i;
    }

    /** Pick a folder (API 21+). Gives access to the whole subtree. */
    public static Intent buildOpenTreeIntent() {
        Intent i = new Intent(Intent.ACTION_OPEN_DOCUMENT_TREE);
        i.addFlags(PERSIST_FLAGS
                | Intent.FLAG_GRANT_PERSISTABLE_URI_PERMISSION
                | Intent.FLAG_GRANT_PREFIX_URI_PERMISSION);
        return i;
    }

    // ---------------------------------------------------------------------
    // 2) Result handling (C++ calls this from the activity-result callback)
    // ---------------------------------------------------------------------

    /**
     * Extracts the picked URI(s) from the result Intent and optionally
     * takes a persistable permission so the URI still works after a restart.
     */
    public static String[] extractUris(Context ctx, Intent data, boolean persist) {
        List<String> out = new ArrayList<>();
        try {
            if (data == null) return new String[0];
            int grantFlags = data.getFlags()
                    & (Intent.FLAG_GRANT_READ_URI_PERMISSION | Intent.FLAG_GRANT_WRITE_URI_PERMISSION);

            ClipData clip = data.getClipData();
            if (clip != null) {                               // multiple selection
                for (int i = 0; i < clip.getItemCount(); ++i) {
                    addUri(ctx, clip.getItemAt(i).getUri(), grantFlags, persist, out);
                }
            } else if (data.getData() != null) {              // single selection
                addUri(ctx, data.getData(), grantFlags, persist, out);
            }
        } catch (Exception e) {
            Log.e(TAG, "extractUris failed", e);
        }
        return out.toArray(new String[0]);
    }

    private static void addUri(Context ctx, Uri uri, int grantFlags, boolean persist, List<String> out) {
        if (uri == null) return;
        if (persist) {
            try {
                ctx.getContentResolver().takePersistableUriPermission(uri, grantFlags);
            } catch (SecurityException e) {
                // Provider does not offer persistable grants; URI is still valid for this session.
                Log.w(TAG, "Cannot persist permission for " + uri + ": " + e.getMessage());
            }
        }
        out.add(uri.toString());
    }

    // ---------------------------------------------------------------------
    // 3) Persisted permissions (remember last folder/file across app starts)
    // ---------------------------------------------------------------------

    public static String[] getPersistedUris(Context ctx) {
        List<String> out = new ArrayList<>();
        try {
            for (UriPermission p : ctx.getContentResolver().getPersistedUriPermissions()) {
                out.add(p.getUri().toString());
            }
        } catch (Exception e) {
            Log.e(TAG, "getPersistedUris failed", e);
        }
        return out.toArray(new String[0]);
    }

    public static void releasePermission(Context ctx, String uriStr) {
        try {
            ctx.getContentResolver().releasePersistableUriPermission(Uri.parse(uriStr), PERSIST_FLAGS);
        } catch (Exception e) {
            Log.w(TAG, "releasePermission failed: " + e.getMessage());
        }
    }

    // ---------------------------------------------------------------------
    // 4) Metadata
    // ---------------------------------------------------------------------

    public static String getDisplayName(Context ctx, String uriStr) {
        return queryString(ctx, uriStr, OpenableColumns.DISPLAY_NAME);
    }

    public static long getSize(Context ctx, String uriStr) {
        String s = queryString(ctx, uriStr, OpenableColumns.SIZE);
        try { return s == null ? -1 : Long.parseLong(s); } catch (NumberFormatException e) { return -1; }
    }

    public static String getMimeType(Context ctx, String uriStr) {
        try { return ctx.getContentResolver().getType(Uri.parse(uriStr)); }
        catch (Exception e) { return null; }
    }

    private static String queryString(Context ctx, String uriStr, String column) {
        Cursor c = null;
        try {
            c = ctx.getContentResolver().query(Uri.parse(uriStr), new String[]{column}, null, null, null);
            if (c != null && c.moveToFirst() && !c.isNull(0)) return c.getString(0);
        } catch (Exception e) {
            Log.w(TAG, "query failed: " + e.getMessage());
        } finally {
            if (c != null) c.close();
        }
        return null;
    }

    // ---------------------------------------------------------------------
    // 5) I/O  -- the key part for Qt
    // ---------------------------------------------------------------------

    /**
     * Opens the document and returns a *detached* native file descriptor.
     * Ownership moves to the caller: in Qt use QFile::open(fd, mode, AutoCloseHandle).
     *
     * mode: "r" (read), "wt" (write + truncate), "rw", "rwt".
     * Prefer "wt" over "w": some providers do not truncate with plain "w".
     * Returns -1 on failure.
     */
    public static int openFd(Context ctx, String uriStr, String mode) {
        try {
            ParcelFileDescriptor pfd = ctx.getContentResolver().openFileDescriptor(Uri.parse(uriStr), mode);
            return pfd == null ? -1 : pfd.detachFd();
        } catch (Exception e) {
            Log.e(TAG, "openFd failed for " + uriStr + ": " + e.getMessage());
            return -1;
        }
    }

    /**
     * Copies the document into the app cache and returns the absolute path of
     * the copy, or null on failure. Useful for libraries that only accept paths.
     * (Costs disk space and time for big files: prefer openFd when you can.)
     */
    public static String copyToCache(Context ctx, String uriStr) {
        InputStream in = null;
        OutputStream out = null;
        try {
            String name = getDisplayName(ctx, uriStr);
            if (name == null || name.isEmpty()) name = "saf_" + System.currentTimeMillis();
            name = name.replaceAll("[\\\\/:*?\"<>|]", "_");

            File dir = new File(ctx.getCacheDir(), "saf");
            if (!dir.exists() && !dir.mkdirs()) return null;
            File dst = new File(dir, name);

            ContentResolver cr = ctx.getContentResolver();
            in = cr.openInputStream(Uri.parse(uriStr));
            if (in == null) return null;
            out = new FileOutputStream(dst);
            byte[] buf = new byte[64 * 1024];
            int n;
            while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
            out.flush();
            return dst.getAbsolutePath();
        } catch (Exception e) {
            Log.e(TAG, "copyToCache failed: " + e.getMessage());
            return null;
        } finally {
            try { if (in != null) in.close(); } catch (Exception ignored) {}
            try { if (out != null) out.close(); } catch (Exception ignored) {}
        }
    }

    // ---------------------------------------------------------------------
    // 6) Folder access (after buildOpenTreeIntent)
    // ---------------------------------------------------------------------

    /**
     * Lists the direct children of a tree URI (or of a sub-folder URI returned
     * by an earlier call). Each entry is "uri \t displayName \t mimeType \t isDir(0|1)".
     */
    public static String[] listChildren(Context ctx, String uriStr) {
        List<String> out = new ArrayList<>();
        Cursor c = null;
        try {
            Uri uri = Uri.parse(uriStr);
            String parentId = DocumentsContract.isDocumentUri(ctx, uri)
                    ? DocumentsContract.getDocumentId(uri)
                    : DocumentsContract.getTreeDocumentId(uri);
            Uri children = DocumentsContract.buildChildDocumentsUriUsingTree(uri, parentId);

            c = ctx.getContentResolver().query(children, new String[]{
                    DocumentsContract.Document.COLUMN_DOCUMENT_ID,
                    DocumentsContract.Document.COLUMN_DISPLAY_NAME,
                    DocumentsContract.Document.COLUMN_MIME_TYPE}, null, null, null);
            if (c == null) return new String[0];

            while (c.moveToNext()) {
                String id = c.getString(0);
                String name = c.getString(1);
                String mime = c.getString(2);
                boolean isDir = DocumentsContract.Document.MIME_TYPE_DIR.equals(mime);
                Uri docUri = DocumentsContract.buildDocumentUriUsingTree(uri, id);
                out.add(docUri + "\t" + name + "\t" + mime + "\t" + (isDir ? "1" : "0"));
            }
        } catch (Exception e) {
            Log.e(TAG, "listChildren failed: " + e.getMessage());
        } finally {
            if (c != null) c.close();
        }
        return out.toArray(new String[0]);
    }

    /** Creates a new file inside a folder. Returns the new document URI or null. */
    public static String createFileInTree(Context ctx, String parentUriStr, String mimeType, String displayName) {
        try {
            Uri parent = Uri.parse(parentUriStr);
            if (!DocumentsContract.isDocumentUri(ctx, parent)) {
                // plain tree URI -> convert to the root document URI of that tree
                parent = DocumentsContract.buildDocumentUriUsingTree(
                        parent, DocumentsContract.getTreeDocumentId(parent));
            }
            Uri created = DocumentsContract.createDocument(
                    ctx.getContentResolver(), parent, mimeType, displayName);
            return created == null ? null : created.toString();
        } catch (Exception e) {
            Log.e(TAG, "createFileInTree failed: " + e.getMessage());
            return null;
        }
    }

    public static boolean deleteDocument(Context ctx, String uriStr) {
        try { return DocumentsContract.deleteDocument(ctx.getContentResolver(), Uri.parse(uriStr)); }
        catch (Exception e) { return false; }
    }

    // ---------------------------------------------------------------------
    // 7) "Save As" done properly: folder picker + own name check
    //
    //    Flow:  buildPickFolderIntent -> user picks folder
    //           -> app asks for file name (own dialog)
    //           -> resolveSaveTarget()  => "new" | "same" | "exists" | "error"
    //                "new"    : createFileExact()  then write
    //                "same"   : write silently into the current document
    //                "exists" : ask user; if yes write into the returned URI (openFd "wt")
    // ---------------------------------------------------------------------

    /** System folder chooser; opens at initialUri (e.g. last used folder) on API 26+. */
    public static Intent buildPickFolderIntent(String initialUri) {
        Intent i = buildOpenTreeIntent();
        if (initialUri != null && !initialUri.isEmpty() && Build.VERSION.SDK_INT >= 26) {
            try {
                i.putExtra(DocumentsContract.EXTRA_INITIAL_URI, Uri.parse(initialUri));
            } catch (Exception ignored) {}
        }
        return i;
    }

    /** True if both URIs point at the same underlying document (same provider + document id). */
    public static boolean isSameDocument(Context ctx, String a, String b) {
        try {
            if (a == null || b == null || a.isEmpty() || b.isEmpty()) return false;
            Uri ua = Uri.parse(a), ub = Uri.parse(b);
            if (ua.equals(ub)) return true;
            String authA = ua.getAuthority(), authB = ub.getAuthority();
            if (authA == null || !authA.equals(authB)) return false;
            return DocumentsContract.getDocumentId(ua).equals(DocumentsContract.getDocumentId(ub));
        } catch (Exception e) {
            return false;
        }
    }

    /**
     * Decides what a Save As to (folder, name) means.
     * Returns String[3] = { status, existingUri, existingName }
     *   status "new"    : no such file in that folder
     *   status "same"   : file exists and IS the document currently open in the app
     *   status "exists" : file exists and is a different document -> ask the user
     *   status "error"  : bad name / folder not accessible
     */
    public static String[] resolveSaveTarget(Context ctx, String folderUri, String name, String currentUri) {
        try {
            if (name == null || name.trim().isEmpty() || name.contains("/") || name.contains("\\")) {
                return new String[]{"error", "", ""};
            }
            String[] hit = findChild(ctx, Uri.parse(folderUri), name.trim());
            if (hit == null) return new String[]{"new", "", ""};
            boolean same = isSameDocument(ctx, hit[0], currentUri);
            return new String[]{same ? "same" : "exists", hit[0], hit[1]};
        } catch (Exception e) {
            Log.e(TAG, "resolveSaveTarget failed: " + e.getMessage());
            return new String[]{"error", "", ""};
        }
    }

    /**
     * Creates a file with EXACTLY this display name (no "(1)" numbering, no forced extension).
     * Call only after resolveSaveTarget() said "new". Returns the document URI, or null.
     * If the provider altered the name (extension appended, race with another writer)
     * it tries to rename; if that fails the half-created file is deleted and null is returned.
     */
    public static String createFileExact(Context ctx, String folderUri, String mimeType, String name) {
        ContentResolver cr = ctx.getContentResolver();
        Uri created = null;
        try {
            name = name.trim();
            Uri folder = Uri.parse(folderUri);
            Uri parent = DocumentsContract.buildDocumentUriUsingTree(folder, docIdOf(ctx, folder));
            String mime = (mimeType == null || mimeType.isEmpty()) ? "application/octet-stream" : mimeType;

            created = DocumentsContract.createDocument(cr, parent, mime, name);
            if (created == null) return null;

            String actual = getDisplayName(ctx, created.toString());
            if (!name.equals(actual)) {
                Uri renamed = DocumentsContract.renameDocument(cr, created, name);
                if (renamed != null) created = renamed;
                actual = getDisplayName(ctx, created.toString());
                if (!name.equals(actual)) {
                    DocumentsContract.deleteDocument(cr, created);   // our own empty file
                    return null;
                }
            }
            return created.toString();
        } catch (Exception e) {
            Log.e(TAG, "createFileExact failed: " + e.getMessage());
            try { if (created != null) DocumentsContract.deleteDocument(cr, created); } catch (Exception ignored) {}
            return null;
        }
    }

    // ---- helpers ----

    private static String docIdOf(Context ctx, Uri uri) {
        return DocumentsContract.isDocumentUri(ctx, uri)
                ? DocumentsContract.getDocumentId(uri)
                : DocumentsContract.getTreeDocumentId(uri);
    }

    /** Finds a direct child by display name. Returns { uri, displayName } or null. */
    private static String[] findChild(Context ctx, Uri folder, String wanted) {
        Cursor c = null;
        String[] caseInsensitiveHit = null;
        try {
            Uri children = DocumentsContract.buildChildDocumentsUriUsingTree(folder, docIdOf(ctx, folder));
            c = ctx.getContentResolver().query(children, new String[]{
                    DocumentsContract.Document.COLUMN_DOCUMENT_ID,
                    DocumentsContract.Document.COLUMN_DISPLAY_NAME}, null, null, null);
            if (c == null) return null;
            while (c.moveToNext()) {
                String id = c.getString(0);
                String n = c.getString(1);
                if (n == null) continue;
                String[] entry = { DocumentsContract.buildDocumentUriUsingTree(folder, id).toString(), n };
                if (n.equals(wanted)) return entry;                       // exact match wins
                // shared storage is case-insensitive: "A.txt" and "a.txt" are the same file there
                if (caseInsensitiveHit == null && n.equalsIgnoreCase(wanted)) caseInsensitiveHit = entry;
            }
        } finally {
            if (c != null) c.close();
        }
        return caseInsensitiveHit;
    }
}
