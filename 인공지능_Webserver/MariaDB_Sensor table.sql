CREATE TABLE `sensor` (
  `ID` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `temp` int(11) DEFAULT 0,
  `humidity` int(10) unsigned DEFAULT 0,
  `cdc` int(10) unsigned DEFAULT 0,
  `water` int(10) unsigned DEFAULT 0,
  `co2` int(10) unsigned DEFAULT 0,
  `date` timestamp NULL DEFAULT current_timestamp(),
  PRIMARY KEY (`ID`)
) ENGINE=InnoDB AUTO_INCREMENT=10174 DEFAULT CHARSET=utf8mb4