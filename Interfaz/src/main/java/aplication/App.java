package aplication;

import javafx.application.Application;
import javafx.fxml.FXMLLoader;
import javafx.scene.Scene;
import javafx.scene.layout.AnchorPane;
import javafx.stage.Stage;
import utils.Paths;

public class App extends Application {


    @Override
    public void start(Stage stage) throws Exception {
        FXMLLoader loader = new FXMLLoader(getClass().getResource("/Interfaz.fxml"));
        System.out.println(">>> PATH CONST: [" + Paths.INTERFAZ_SUMA_VIEW + "]");
        System.out.println(">>> RUTA getResource: " + getClass().getResource(Paths.INTERFAZ_SUMA_VIEW));

        AnchorPane pane = (AnchorPane) loader.load();
        Scene scene = new Scene(pane);
        stage.setScene(scene);
        stage.show();
    }

    public static void main(String[] args) {
        launch();
    }
}
