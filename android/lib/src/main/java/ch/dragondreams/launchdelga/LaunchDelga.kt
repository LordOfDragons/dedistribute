package ch.dragondreams.launchdelga

import android.app.Activity
import android.content.ActivityNotFoundException
import android.content.DialogInterface
import android.content.Intent
import android.util.Log
import androidx.appcompat.app.AlertDialog

/**
 * Launch DELGA file using the Drag[en]gine Game Engine Launcher.
 *
 * Use like this:
 * ```
 * val launchDelga = LaunchDelga(this, "$packageName.provider", "ddtestproduct.delga")
 * launchDelga.launch()
*  ```
 */
class LaunchDelga(
    /**
     * Required for launching the DELGA file and it is also
     * used to show UI elements to assist the user in case the
     * Drag[en]gine Game Engine Launcher app is not yet installed.
     */
    val activity: Activity,

    /**
     * Authority for launching the DELGA file. It is recommended to use
     * "$packageName.provider".
     */
    var authority: String,

    /**
     * DELGA asset filename. DELGA file has to be located inside the assets directory.
     * Make sure the DELGA file is included in the APK uncompressed.
     */
    val delgaAssetFilename: String
) {
    /**
     * Launch DELGA file using the store settings.
     *
     * If the launching succeeds Activity.finish() is called and the activity stops.
     *
     * In case of failure an UI error dialog is shown. The caller has to do nothing.
     * Once the dialog is closed Activity.finish() is called or launch() is called again.
     *
     * In all situations the caller has to do nothing after returning from the function.
     */
    fun launch() {
        Log.i(TAG, "launch: authority='$authority' file='${delgaAssetFilename}'")

        val intent = Intent()
        intent.action = ACTION_LAUNCH_DELGA
        intent.setDataAndType(
            DelgaAssetProvider.getUriForFile(delgaAssetFilename, authority),
            DelgaAssetProvider.DELGA_MIME_TYPE)
        intent.addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)

        try {
            activity.startActivity(intent)
            activity.finish()

        } catch (e: ActivityNotFoundException) {
            activity.runOnUiThread {
                if (e.message != null) {
                    showError(e.message!!, "Launch DELGA")
                } else {
                    showError("Failed running DELGA", "Launch DELGA")
                }
            }
        }
    }

    /**
     * Show error dialog.
     */
    fun showError(message: String, title: String){
        val builder = AlertDialog.Builder(activity)
        builder.setMessage(message)
        builder.setTitle(title)
        builder.setCancelable(true) // can close clicking outside dialog
        builder.setPositiveButton("Close") { dialog: DialogInterface?, _: Int ->
            dialog?.cancel()
            activity.finish()
        }
        builder.create().show()
    }

    companion object {
        private const val TAG = "LaunchableDelga"

        const val ACTION_LAUNCH_DELGA = "ch.dragondreams.delauncher.LAUNCH_DELGA"
    }
}
