//Allen Zou
//11/04/2020

const fs = require("fs");
const path = require("path");
const tingodb = require("tingodb");
const Engine = tingodb();
const collection_name = "smoke_records";
const db_folder = path.join(__dirname, "smoke.db")
const db = new Engine.Db(db_folder, {});

function populateDatabase() {
    const collection = db.createCollection(collection_name);
    const data = fs.readFileSync(path.join(__dirname, "smoke.txt")).toString();

    let header = null
    const records = data.split("\n").reduce(function (accum, line, idx) {
        if (!line.trim()) {
            // Do nothing
        }
        else if (header == null) {
            header = line.split("\t").map(c => c.trim());
        }
        else {
            let record = line.split("\t").reduce((obj, column, idx) => {
                obj[header[idx]] = parseFloat(column.trim());
                return obj;
            }, {});
            accum.push(record);
        }

        return accum;
    }, []);

    db.open(function (err, item) {
        if (err) {
            console.error("Error opening database: ", err);
            return;
        }

        collection.insert(records, function (err, item) {
            if (err) {
                console.error("Error inserting records", err);
            }

            console.log("Created database!");
        });
    })
}


/**
 * An quick and dirty function to run a simple query on the database.  */
function exampleQuery() {
    exports.records.find({Smoke: 0}).toArray(function (err, data) {
        console.log(data);
    });
}


exports.populate = populateDatabase;
exports.db = db;
exports.records = db.collection(collection_name);

exports.exampleQuery = exampleQuery;

if (require.main == module) {
    if (!fs.existsSync(db_folder)) {
        fs.mkdirSync(db_folder);
    }

    if (!fs.existsSync(path.join(db_folder, collection_name))) {
        populateDatabase(); 
    }
    else {
        exampleQuery();
    }
}
