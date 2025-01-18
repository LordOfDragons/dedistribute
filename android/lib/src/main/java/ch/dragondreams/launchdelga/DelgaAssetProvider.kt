package ch.dragondreams.launchdelga

import android.content.ContentProvider
import android.content.ContentValues
import android.content.res.AssetFileDescriptor
import android.database.Cursor
import android.net.Uri
import android.util.Log
import java.io.IOException


class DelgaAssetProvider : ContentProvider() {
    override fun onCreate(): Boolean {
        return true
    }

    override fun openAssetFile(uri: Uri, mode: String): AssetFileDescriptor? {
        val assets = context?.assets ?: throw NullPointerException("context")
        val path = uri.pathSegments.joinToString("/")
        var afd: AssetFileDescriptor? = null
        try {
            afd = assets.openFd(path)
        } catch (e: IOException) {
            Log.e(TAG, Log.getStackTraceString(e))
        }
        return afd
    }

    override fun query(uri: Uri, projection: Array<out String>?, selection: String?,
                       selectionArgs: Array<out String>?, sortOrder: String?): Cursor? {
        return null
    }

    override fun getType(uri: Uri): String? {
        return if (uri.lastPathSegment?.endsWith(".delga") == true) {
            DELGA_MIME_TYPE
        }else{
            "application/octet-stream"
        }
    }

    override fun insert(uri: Uri, values: ContentValues?): Uri? {
        return null
    }

    override fun delete(uri: Uri, selection: String?, selectionArgs: Array<out String>?): Int {
        return 0
    }

    override fun update(uri: Uri, values: ContentValues?, selection: String?,
                        selectionArgs: Array<out String>?): Int {
        return 0
    }

    companion object {
        const val TAG = "DelgaAssetProvider"

        /**
         * DELGA files mime type.
         */
        const val DELGA_MIME_TYPE = "application/dragengine-delga"

        /**
         * Get URI for file to be used for intent.
         */
        fun getUriForFile(delgaFilename: String, authority: String): Uri {
            return Uri.Builder()
                .scheme("content")
                .authority(authority)
                .appendPath(delgaFilename)
                .build()
        }
    }
}
