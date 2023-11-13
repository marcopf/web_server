<?php
if ($_SERVER["REQUEST_METHOD"] == "POST") {
    $file = $_FILES["fileToUpload"];

    if ($file["error"] === UPLOAD_ERR_OK) {
        $uploadedFileName = $file["name"];
        $tempFilePath = $file["tmp_name"];
        
        // Fai qualcosa con il file, ad esempio spostalo in una directory definita
        move_uploaded_file($tempFilePath, "uploads/" . $uploadedFileName);
        
        echo "Il file $uploadedFileName è stato caricato con successo.";
    } else {
        echo "Errore durante il caricamento del file.";
    }
}
?>