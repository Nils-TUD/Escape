<?php
define('FWS_PATH','/var/www/scriptsolution/PHPLib/');
include(FWS_PATH.'init.php');

$license = <<<EOF
/**
 * \$Id\$
 * Copyright (C) 2008 - 2009 Nils Asmussen
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */
EOF;

foreach(FWS_FileUtils::get_list('.',true,true) as $item)
{
	if(preg_match('/^[a-z0-9_\.]+?\.(c|h)$/',basename($item)))
	{
		$s = implode('',file($item));
		$s = preg_replace('/^\/\*\*.+?\*\//s',$license,$s);
		FWS_FileUtils::write($item,$s);
	}
}
?>
