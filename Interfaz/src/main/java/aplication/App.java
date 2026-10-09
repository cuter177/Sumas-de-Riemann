package aplication;

import controllers.InterfazController;
import javafx.application.Application;
import javafx.fxml.FXMLLoader;
import javafx.scene.Parent;
import javafx.scene.Scene;
import javafx.scene.paint.Color;
import javafx.stage.Stage;
import javafx.stage.StageStyle;

public class App extends Application {

    // Tamaño mínimo razonable para que el layout nunca se recorte.
    private static final double MIN_ANCHO  = 460;
    private static final double MIN_ALTO   = 420;

    @Override
    public void start(Stage stage) throws Exception {
        FXMLLoader loader = new FXMLLoader(getClass().getResource("/Interfaz.fxml"));
        Parent root = loader.load();

        boolean chromePersonalizado = usarChromePersonalizado();
        System.out.println("[UI] Estilo de ventana: "
                + (chromePersonalizado ? "personalizado (transparente)" : "decorado (nativo)"));

        Scene scene = new Scene(root);
        // Solo tiene sentido un fondo transparente si usamos chrome propio.
        if (chromePersonalizado) {
            scene.setFill(Color.TRANSPARENT);
        }

        stage.setTitle("Sumas de Riemann");
        stage.setResizable(true);
        stage.setMinWidth(MIN_ANCHO);
        stage.setMinHeight(MIN_ALTO);
        stage.initStyle(chromePersonalizado ? StageStyle.TRANSPARENT : StageStyle.DECORATED);
        stage.setScene(scene);

        InterfazController controller = loader.getController();
        controller.configurarVentana(stage, chromePersonalizado);

        stage.centerOnScreen();
        stage.show();
    }

    /**
     * Decide entre chrome personalizado (ventana transparente sin bordes) y
     * ventana decorada por el gestor de ventanas.
     *
     * - Transparente solo si hay compositor y un escritorio "flotante" conocido
     *   (GNOME, KDE, XFCE...). En gestores de mosaico (i3, sway, Hyprland...) o
     *   entornos desconocidos se usa la decoración nativa, que es la que mejor
     *   se integra con el tileo.
     * - Se puede forzar con la variable de entorno RIEMANN_WINDOW_STYLE:
     *   "decorated" o "transparent".
     */
    private boolean usarChromePersonalizado() {
        String override = System.getenv("RIEMANN_WINDOW_STYLE");
        if (override != null) {
            override = override.trim().toLowerCase();
            if (override.equals("decorated") || override.equals("decorada")) return false;
            if (override.equals("transparent") || override.equals("transparente")) return true;
        }

        // Windows y macOS siempre tienen compositor: chrome personalizado.
        String os = System.getProperty("os.name", "").toLowerCase();
        if (!os.contains("linux")) return true;

        // En Linux solo para escritorios flotantes con compositor.
        String de = primeroNoVacio(
                        System.getenv("XDG_CURRENT_DESKTOP"),
                        System.getenv("XDG_SESSION_DESKTOP"),
                        System.getenv("DESKTOP_SESSION"));
        de = de == null ? "" : de.toLowerCase();

        String[] flotantes = {
            "gnome", "kde", "plasma", "xfce", "cinnamon", "mate", "unity",
            "budgie", "deepin", "pantheon", "lxqt", "pop"
        };
        for (String f : flotantes) {
            if (de.contains(f)) return true;
        }
        // Gestor de mosaico o entorno desconocido -> ventana decorada.
        return false;
    }

    private static String primeroNoVacio(String... valores) {
        for (String v : valores) {
            if (v != null && !v.isBlank()) return v;
        }
        return "";
    }

    public static void main(String[] args) {
        launch();
    }
}
