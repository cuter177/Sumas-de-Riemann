package controllers;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import javafx.event.ActionEvent;
import javafx.fxml.FXML;
import javafx.scene.control.Label;
import javafx.scene.control.TextField;
import javafx.scene.layout.AnchorPane;
import javafx.scene.web.WebView;

import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.util.HashMap;
import java.util.Map;

public class InterfazController {

    @FXML private Label lblDeltax;
    @FXML private Label lblSuma;
    @FXML private TextField txtFuncion;
    @FXML private TextField txtLi;
    @FXML private TextField txtLs;
    @FXML private TextField txtN;
    @FXML private AnchorPane visorLatex;
    @FXML private WebView webViewLatex;


    // campo nuevo
    private boolean paginaLista = false;

    @FXML
    public void initialize() {
        // ajustar anclas (webView viene del FXML)
        AnchorPane.setTopAnchor(webViewLatex, 0.0);
        AnchorPane.setBottomAnchor(webViewLatex, 0.0);
        AnchorPane.setLeftAnchor(webViewLatex, 0.0);
        AnchorPane.setRightAnchor(webViewLatex, 0.0);
        // cargar HTML y MathJax
        cargarPlantillaMathJax();

        // esperar a que la página termine de cargar
        webViewLatex.getEngine().getLoadWorker().stateProperty().addListener((obs, oldS, newS) -> {
            switch (newS) {
                case SUCCEEDED -> {
                    paginaLista = true;
                    // ahora MathJax puede tardar un poco en inicializarse; intentamos renderizar la primera fórmula
                    // llamamos generarLatex() después de un pequeño delay para darle tiempo a MathJax
                    webViewLatex.getEngine().executeScript(
                            "setTimeout(function(){ console.log('intentando primer render'); }, 50);"
                    );
                    generarLatex();
                }
                default -> { }
            }
        });

        // Listeners (no llamamos generarLatex() aquí sin antes cargar la página)
        txtFuncion.textProperty().addListener((o, ov, nv) -> generarLatex());
        txtLi.textProperty().addListener((o, ov, nv) -> generarLatex());
        txtLs.textProperty().addListener((o, ov, nv) -> generarLatex());
        txtN.textProperty().addListener((o, ov, nv) -> generarLatex());
    }

    private void cargarPlantillaMathJax() {

        String mathjax = getClass().getResource("/mathjax/es5/tex-mml-svg.js").toExternalForm();

        String html = """
        <!DOCTYPE html>
        <html>
        <head>
        <meta charset="UTF-8">

        <style>
          html, body {
            height: 100%%;
            margin: 0;
            padding: 0;
          }

          body {
            display: flex;
            justify-content: center;
            align-items: center;
            background-color: white;
          }

          #latex {
            text-align: center;
            font-size: 220%%;
            overflow: visible;
          }
        </style>

        <script>
        window.MathJax = {
          tex: { inlineMath: [['$', '$'], ['\\\\(', '\\\\)']] },
          svg: { fontCache: 'none' }
        };
        </script>

        <script src="%s"></script>
        </head>

        <body>
          <div id="latex">Cargando…</div>
        </body>
        </html>
        """.formatted(mathjax);

        webViewLatex.getEngine().loadContent(html);
    }


    private void actualizarLatex(String latex) {
        if (webViewLatex == null || webViewLatex.getEngine() == null)
            return;

        // Aquí está la clave: duplicar los backslashes una sola vez.
        String safe = latex
                .replace("\\", "\\\\")   // <-- ESCAPE NECESARIO
                .replace("'", "\\'");

        String js = """
    try {
        document.getElementById('latex').innerHTML = '$$' + '%s' + '$$';
        MathJax.typesetClear();
        MathJax.typeset();
    } catch(e) { console.log(e); }
    """.formatted(safe);

        webViewLatex.getEngine().executeScript(js);
    }

    private String normalizarSimbolos(String expr) {
        if (expr == null) return "";

        return expr
                .replaceAll("\\bpi\\b", "\\\\pi")   // pi → \pi
                .replaceAll("\\bPI\\b", "\\\\pi")   // PI → \pi
                .replaceAll("\\bPi\\b", "\\\\pi")   // Pi → \pi
                .replaceAll("\\be\\b", "e");        // e → e (para constante matemática)
    }

    private void generarLatex() {
        String fRaw = txtFuncion.getText();
        String liRaw = txtLi.getText();
        String lsRaw = txtLs.getText();

        // Normalizar símbolos antes de construir el LaTeX
        String f = normalizarSimbolos(fRaw == null || fRaw.isBlank() ? "f(x)" : fRaw);
        String li = normalizarSimbolos(liRaw == null || liRaw.isBlank() ? "a" : liRaw);
        String ls = normalizarSimbolos(lsRaw == null || lsRaw.isBlank() ? "b" : lsRaw);

        String latex = "\\int_{" + li + "}^{" + ls + "} " + f + "\\,\\mathrm{d}x";

        actualizarLatex(latex);
    }

    @FXML
    void guardarJson(ActionEvent event) {

        // Datos a guardar
        String f = txtFuncion.getText();
        String li = txtLi.getText();
        String ls = txtLs.getText();
        String n = txtN.getText();

        Map<String, Object> datos = new HashMap<>();
        datos.put("funcion", f);
        datos.put("li", li);
        datos.put("ls", ls);
        datos.put("numRectangulos", n);

        Gson gson = new GsonBuilder().setPrettyPrinting().create();

        try {
            // Crear carpeta data si no existe
            File carpeta = new File("data");
            carpeta.mkdirs();

            // Archivo Funcion.json
            FileWriter writer = new FileWriter("data/Funcion.json");
            gson.toJson(datos, writer);
            writer.flush();
            writer.close();

            System.out.println("Funcion.json guardado correctamente.");
            System.out.println("[JAVA] Escribiendo: "
                    + new File("data/Funcion.json").getAbsolutePath());

            // === ESPERAR A QUE C++ GENERE Resultado.json ===
            // (El programa C++ lo creará en menos de 200 ms)
            Thread.sleep(250);

            // === LEER Resultado.json ===
            cargarResultadoJson();

        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    void cargarResultadoJson() {

        File file = new File("data", "Resultado.json");


        if (!file.exists()) {
            System.out.println("Resultado.json aún no está disponible.");
            return;
        }

        try {
            Gson gson = new Gson();

            FileReader reader = new FileReader(file);
            Map<String, Object> datos = gson.fromJson(reader, Map.class);
            reader.close();

            double resultado = ((Number) datos.get("resultado_integral")).doubleValue();
            double deltax = ((Number) datos.get("delta_x")).doubleValue();

            lblSuma.setText("Resultado de la sumatoria: " + String.valueOf(resultado));
            lblDeltax.setText("Delta de x: " + String.valueOf(deltax));

            System.out.println("Resultado.json leído correctamente.");

        } catch (Exception e) {
            e.printStackTrace();
        }
    }

}


