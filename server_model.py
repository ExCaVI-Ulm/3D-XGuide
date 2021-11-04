import io
import os
import time

import flask
from PIL import Image
from tensorflow.keras.models import load_model
import numpy as np


# initialize our Flask application and the Keras model
app = flask.Flask(__name__)
model = None


def model_load():
    # load the pre-trained Keras model (here we are using a model
    # pre-trained on ImageNet and provided by Keras, but you can
    # substitute in your own networks just as easily)
    global model_x
    global model_y
    model_y = load_model("netY.h5")
    model_x = load_model("netX.h5")
    #model_y = load_model(os.path.join("b:","myNet10epoch16batchScaling256dist2-dist1.h5"))
    #model_x = load_model(os.path.join("b:","myNet10epoch16batchScaling256dist2-dist1_x.h5"))

def prepare_images(im1, im2):

    # classify the input image and then initialize the list
    # of predictions to return to the client
    arr_images = np.empty((1, 512, 512, 2))
    images = np.empty((512, 512, 2))
    # read images
    image1 = np.array(im1)
    image2 = np.array(im2)

    images[:, :, 0] = image1
    images[:, :, 1] = image2
    arr_images[0,] = images
    arr_images = arr_images.astype('float32')
    arr_images /= 255

    # return the processed images
    return arr_images

@app.route("/status", methods=["GET"])
def status():
    # initialize the data dictionary that will be returned from the
    # view
    data = {"success": True}
    return flask.jsonify(data)

def predict(model, string):
    prediction_time_start = time.process_time()
    # initialize the data dictionary that will be returned from the
    # view
    data = {"success": False}

    # ensure an image was properly uploaded to our endpoint
    if flask.request.method == "POST":
        if flask.request.files.get("image1") and flask.request.files.get("image2"):
            # read the image in PIL format
            image1 = flask.request.files["image1"].read()
            image1 = Image.open(io.BytesIO(image1))

            image2 = flask.request.files["image2"].read()
            image2 = Image.open(io.BytesIO(image2))

            result, time_to_predict = do_prediction(image1, image2, model)
            if string == 'x':
                largest_value_scaled = 137
                smallest_value_scaled = -137
            if string == 'y':
                largest_value_scaled = 66
                smallest_value_scaled = -66
            result_scaled = result * (largest_value_scaled - smallest_value_scaled) + smallest_value_scaled
            data["prediction_" + string] = str(result_scaled.astype(np.int))

            # indicate that the request was a success
            data["success"] = True

            prediction_time_stop = time.process_time()
            data["prediction time [s]"] = str(time_to_predict)
            data["run time [s]"] = str(prediction_time_stop - prediction_time_start)
    # print("Run time per frame [s]: {}".format(
    #     prediction_time_stop - prediction_time_start))
    # return the data dictionary as a JSON response
    return flask.jsonify(data)


def do_prediction(image1, image2, model):
    #largest_value_scaled = 256
    #smallest_value_scaled = -256
    # preprocess the image and prepare it for prediction
    images = prepare_images(image1, image2)
    prediction_time_start = time.process_time()
    predict_res = model.predict(images)
    prediction_time_stop = time.process_time()
    prediction_time = prediction_time_stop - prediction_time_start
    #print("Prediction time per frame [s]: {}".format(prediction_time))
    #result = predict_res * (largest_value_scaled - smallest_value_scaled) + smallest_value_scaled
    #return result[0][0].astype(np.int), prediction_time
    return predict_res[0][0], prediction_time


@app.route("/predict_x", methods=["POST"])
def predict_x():
    return predict(model_x, 'x')

@app.route("/predict_y", methods=["POST"])
def predict_y():
    return predict(model_y, 'y')

@app.route("/predict_all", methods=["POST"])
def predict_all():
    prediction_time_start = time.process_time()
    # initialize the data dictionary that will be returned from the
    # view
    data = {"success": False}

    # ensure an image was properly uploaded to our endpoint
    if flask.request.method == "POST":
        if flask.request.files.get("image1") and flask.request.files.get("image2"):
            # read the image in PIL format
            image1 = flask.request.files["image1"].read()
            image1 = Image.open(io.BytesIO(image1))

            image2 = flask.request.files["image2"].read()
            image2 = Image.open(io.BytesIO(image2))

            result_x, time_x = do_prediction(image1, image2, model_x)
            largest_value_scaled_x = 137
            smallest_value_scaled_x = -137
            result_x_scaled = result_x * (largest_value_scaled_x - smallest_value_scaled_x) + smallest_value_scaled_x
            data["prediction_x"] = str(result_x_scaled.astype(np.int))
            result_y, time_y = do_prediction(image1, image2, model_y)
            largest_value_scaled_y = 66
            smallest_value_scaled_y = -66
            result_y_scaled = result_y * (largest_value_scaled_y - smallest_value_scaled_y) + smallest_value_scaled_y
            data["prediction_y"] = str(result_y_scaled.astype(np.int))

            # indicate that the request was a success
            data["success"] = True

            prediction_time_stop = time.process_time()
            data["prediction time [s]"] = str(time_x + time_y)
            data["run time [s]"] = str(prediction_time_stop - prediction_time_start)
    # print("Run time per frame [s]: {}".format(
    #     prediction_time_stop - prediction_time_start))
    # return the data dictionary as a JSON response
    return flask.jsonify(data)


# if this is the main thread of execution first load the model and
# then start the server
if __name__ == "__main__":
    print(("* Loading Keras model and Flask starting server..."
        "please wait until server has fully started"))
    model_load()
    app.run()